# Copyright (C) 2021-2022 Victor Suarez Rovere <suarezvictor@gmail.com>
# clang API: https://github.com/llvm-mirror/clang/blob/master/bindings/python/clang/cindex.py


import sys
import clang
import clang.cindex
clang.cindex.Config.set_library_path("/usr/lib/x86_64-linux-gnu")

from clang.cindex import Cursor, TokenKind, CursorKind
from cflexgenerator import CFlexGenerator, CFlexBasicCPPGenerator

def concat_tokens(tokens, sep=""):
    return sep.join([t.spelling for t in tokens])

binary_op_names = {"+": "add", "-": "sub", "*": "mul", "/": "div", "=": "assign", "==":"eq", "!=":"neq", ">": "gt", "<":"lt", ">=":"gte", "<=":"lte", ">>":"shr", "<<":"shl", "&":"and", "|":"or", "^":"xor", "||":"logical_or", "&&":"logical_and" }

def operator_to_name(op):
    return binary_op_names[op]

def recurse_unexposed(c):
    if c.kind not in [CursorKind.UNEXPOSED_EXPR, CursorKind.MEMBER_REF_EXPR]:
        return c
    for ch in c.get_children():
        #return recurse_unexposed(ch) #commented since recursivity leads to the wrong type
        return ch

def bin_operator_to_name(ltyp, op, rtyp):
    rtyp = "_" + rtyp if ltyp != rtyp else ""
    try:
         opname = operator_to_name(op)
    except:
         opname = "UNKNOWN_OPERATOR:"+op;

    return ltyp + "_" + opname + rtyp


def remove_type_qualifiers(typname):
    if typname.endswith("&"):
        return remove_type_qualifiers(typname[:-2])
    if typname.startswith("const "):
        return remove_type_qualifiers(typname[6:])
    if typname.startswith("volatile "):
        return remove_type_qualifiers(typname[9:])
    return typname

def mangle_type(typname):
    return typname.replace(" ", "_")

UNARY_OPERATORS = ["-", "!", "~", "--", "++", "&", "*"]
COMPARISON_OPERATORS = ["==", "!=", ">", "<", ">=", "<="]

class CFlexClangParser:
    def __init__(self, file, args):
        index = clang.cindex.Index.create()
        self.tu = index.parse(file, args)
        self.comments_enabled = True #output comments enabled/disabled 
        self.overloaded_types = ["float"]
        self.vector_overloaded_operators = True
        self.overloaded_operators = True
        self.no_parse_cursors = False #only comments
        self.reverse_typedef_map = {}
        self.cursor_cache = {}

    def empty_childs(self, childs):
        prev_comments_enabled = self.comments_enabled
        self.comments_enabled = False  # temporarily disable comments

        for ch in childs:
            s = self.generate_cursor(ch).strip()
            if len(s) > 0:
                return False

        self.comments_enabled = prev_comments_enabled
        return True

    def leftmost_child(self, c):
        for ch in c.get_children():
            chi = self.leftmost_child(ch)
            if chi.location.line == 0:
                break; #ignore expressions with no line numbers
            if chi.location.line < c.location.line:
                return chi
            if (
                chi.location.line == c.location.line
                and chi.location.column < c.location.column
            ):
                return chi
        return c

    def find_operator_before(self, c, child):
        op = None
        leftmost_rhs = self.leftmost_child(child)
        for t in c.get_tokens():
            if (
                leftmost_rhs.location.line == t.location.line
                and leftmost_rhs.location.column == t.location.column
            ):
                break
            if t.kind == TokenKind.PUNCTUATION:
                op = t.spelling  # last before child location is PUNCTUATION operator
        return op

    def generate_cursor(
        self,
        c,
        k0=[],
        skip=[],
    ):
        if c in skip:
            return ""

        prev_comments_enabled = self.comments_enabled
        """
        try:
          s = self.generate_cursor_internal(c, k0)
        except:
          s = "\n#warning PARSER EXCEPTION\n"
        """
        s = self.generate_cursor_internal(c, k0)
        self.comments_enabled = prev_comments_enabled
        return s

    def childs_recurse(self, args, k0, skip=[]):
        for k, ch in enumerate(args):
            yield self.generate_cursor(ch, k0+[str(k)], skip)

    def generate_cursor_internal(self, c, k0):
        s = ""

        childs = list(c.get_children())
        childrepr = self.childs_recurse(childs, k0)

        tokens = []
        for t in c.get_tokens():
            tokens += [t]
            if (
                len(tokens) > 15
            ):  # puts a limit on recursivity. FIXME: do a function to lookup those needed
                break

        linecol = str(c.location.line) + ":" + str(c.location.column)
        comment = ""
        if self.comments_enabled:
            order = ".".join(k0)
            comment = (
                "\t//"
                + order
                + "["
                + str(len(childs))
                + "] "
                + str(c.kind)
                + " " + str(c.type.spelling)
                + "  line "
                + linecol
            )
            comment += (
                ' "' + c.spelling + '" ' + "/".join([t.spelling for t in tokens]) + "\n"
            )
            #comment = "\t\t\t//" + linecol + "\n"

        #implement basic cache to avoid duplicate generation
        if linecol in self.cursor_cache:
           return self.cursor_cache[linecol]

        s = self.onANY_KIND(c, childs, childrepr, tokens)
        if s is None:
          s = self.generator.generate_any_expr(childrepr)
          
        is_assign = (c.kind == CursorKind.BINARY_OPERATOR and self.find_operator_before(c, childs[1]) == "=")
        silent_expr = c.kind not in [CursorKind.BINARY_OPERATOR, CursorKind.UNEXPOSED_EXPR, CursorKind.DECL_REF_EXPR]
        if silent_expr or is_assign:

            prev, var = self.generator.pop_prev_statements()
            if var is not None:
                s = prev + s
        
        s = self.generator.generate_comment(str(c.kind), comment) + s
        self.cursor_cache[linecol] = s

        return s

    def generate_cursor_clean(self, c):
        prev_comments_enabled = self.comments_enabled
        self.comments_enabled = False
        s = self.generate_cursor(c)
        self.comments_enabled = prev_comments_enabled
        return s

    def first_noncond_typedef(self, typname, cond):
        ctyp = None
        typname = remove_type_qualifiers(typname)
        while cond(typname):
            try:
                cdescendant = self.reverse_typedef_map[mangle_type(typname)]
                ctyp = cdescendant.type
                typname = ctyp.spelling
            except:
                print("typedef not found: ", typname, file=sys.stderr)
                break
        return ctyp


    def get_canonicaltype_and_count(self, cbase, allowtemplate=False):
        def cond(typname):
            return typname[-1] == ">"

        c = recurse_unexposed(cbase)
        ctyp = c.type.get_canonical()
        if not allowtemplate:
            ctypr = self.first_noncond_typedef(ctyp.spelling, cond)
            if ctypr is not None:
                ctyp = ctypr

        typname = ctyp.spelling
        count = 0
        try:
            count = ctyp.element_count
            typname = ctyp.element_type.spelling + str(count)
        except:
            pass
        return mangle_type(typname), count
        
    def onANY_KIND(self, c, childs, childrepr, tokens):
        if self.no_parse_cursors:  # True for skipping all interpretation and call the default for all
            pass
        elif c.kind in [CursorKind.CLASS_TEMPLATE, CursorKind.CLASS_DECL]:
            return self.onCLASS_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.UNION_DECL:
            return self.onUNION_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.STRUCT_DECL:
            return self.onSTRUCT_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.FIELD_DECL:
            return self.onFIELD_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.IF_STMT:
            return self.onIF_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.CONDITIONAL_OPERATOR:
            return self.onCONDITIONAL_OPERATOR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.SWITCH_STMT:
            return self.onSWITCH_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.CASE_STMT:
            return self.onCASE_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.DEFAULT_STMT:
            return self.onDEFAULT_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.MEMBER_REF_EXPR:
            return self.onMEMBER_REF_EXPR(c, childs, childrepr, tokens)
        elif c.kind in [
            CursorKind.FUNCTION_DECL,
            CursorKind.FUNCTION_TEMPLATE,
        ]:  # FIXME: separate
            return self.onFUNCTION_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.COMPOUND_STMT:
            return self.onCOMPOUND_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.RETURN_STMT:
            return self.onRETURN_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.UNEXPOSED_EXPR:
            return self.onUNEXPOSED_EXPR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.DECL_REF_EXPR:
            return self.onDECL_REF_EXPR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.CALL_EXPR:
            return self.onCALL_EXPR(c, childs, childrepr, tokens)
        elif c.kind in [
            CursorKind.INTEGER_LITERAL,
            CursorKind.FLOATING_LITERAL,
            CursorKind.CXX_NULL_PTR_LITERAL_EXPR,
        ]:
            return self.onLITERAL(c, childs, childrepr, tokens)
        elif c.kind in [
            CursorKind.BINARY_OPERATOR,
            CursorKind.COMPOUND_ASSIGNMENT_OPERATOR,  # FIXME: separate
        ]:
            return self.onBINARY_OPERATOR(
                c, childs, childrepr, tokens
            )  # FIXME: separate
        elif c.kind == CursorKind.CXX_FUNCTIONAL_CAST_EXPR:
            return self.onCXX_FUNCTIONAL_CAST_EXPR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.CSTYLE_CAST_EXPR:
            return self.onCSTYLE_CAST_EXPR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.UNARY_OPERATOR:
            return self.onUNARY_OPERATOR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.DECL_STMT:
            return self.onDECL_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.VAR_DECL:
            return self.onVAR_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.TYPEDEF_DECL:
            return self.onTYPEDEF_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.PARM_DECL:
            return self.onPARM_DECL(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.FOR_STMT:
            return self.onFOR_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.TYPE_REF:
            return self.onTYPE_REF(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.INIT_LIST_EXPR:
            return self.onINIT_LIST_EXPR(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.NULL_STMT:
            return self.onNULL_STMT(c, childs, childrepr, tokens)
        elif c.kind == CursorKind.PAREN_EXPR:
            return self.onPAREN_EXPR(c, childs, childrepr, tokens)

        return self.generator.generate_expr(childrepr)

    def onCALL_EXPR(self, c, childs, childrepr, tokens):
        # if len(childs) == 0: #no real calls
        #    return None
        name = c.spelling
        if not name:
            return None  # not a real call

        if name.startswith("operator"):
            op = name[8:]
            #print("OPERATOR", name, "->", op, file=sys.stderr)
            lhschild = self.generate_cursor(childs[0], ["lhs"])
            if len(childs) > 2:
                rhschild = self.generate_cursor(childs[2], ["rhs"])
            else:
                rhschild = None #at least in most situations: casting operator
                #WARNING: evaluating the iterator for debug consumes (iterates)  all elements!!
                #print("OPERATOR", name, "expr", "".join(childrepr), file=sys.stderr)
                #quit()

            if op == "=":
                return self.generator.generate_overloaded_assignment_operator(lhschild, op, rhschild, c.type.spelling) #FIXME: check overload
            ltyp, lcount = self.get_canonicaltype_and_count(childs[0])
            if rhschild:
                rtyp, rcount = self.get_canonicaltype_and_count(childs[2])
                #print("OVERLOADED BINARY OPERATOR:", ltyp, op, rtyp, file=sys.stderr)
                fname = bin_operator_to_name(ltyp, op, rtyp)
                return self.generator.generate_overloaded_call(fname, [lhschild, rhschild], c.typ.spelling)
            elif len(childs) == 1:
                #child0 = childs[0].get_children()
                #ltyp, count = self.get_canonicaltype_and_count(next(child0))
                castname = ltyp + "_to_" + name[9:] #cast
                return self.generator.generate_overloaded_call(castname, childrepr, c.typ.spelling)
            else:
                return self.generator.generate_unary_operator(op, childrepr) 

        if self.overloaded_operators and len(childs) == 1:
            if not c.type.is_pod(): #constructors
                ctyp, count = self.get_canonicaltype_and_count(c)
                ctyparg, count = self.get_canonicaltype_and_count(childs[0])
                if ctyp == ctyparg: #copy constuctor
                    return None
                ctyparg = "_from_" + ctyparg
                name = ctyp + "_make" + ctyparg
                return self.generator.generate_overloaded_call(name, childrepr, c.typ.spelling)
    

        if len(tokens) == 0 or name == tokens[0].spelling:
            argsexpr = self.childs_recurse(childs[1:], ["arg"])
            return self.generator.generate_overloaded_call(name, argsexpr, c.typ.spelling)

        if len(childs) > 1: #for arguments to constructor like in vector init
            argsexpr = self.childs_recurse(childs, ["arg"])
            return self.generator.generate_overloaded_call(name + "_make", argsexpr, c.typ.spelling)
            


    def onCOMPOUND_STMT(self, c, childs, childrepr, tokens):
        #def stmts(expr):
        #    for c in expr:
        #        yield self.generator.generate_stmt(c)
        return self.generator.generate_compound_stmt(childrepr)

    def onFUNCTION_DECL(self, c, childs, childrepr, tokens):
        rettyp = c.result_type.spelling
        name = c.spelling

        args = list(c.get_arguments())
        argsexpr = self.childs_recurse(args, ["arg"])

        stmtexpr = None
        #if len(childs) != len(args):
            #stmtexpr = self.childs_recurse(childs, [""], args)
        if len(childs) > len(args) and childs[-1].kind == CursorKind.COMPOUND_STMT:
            stmtexpr = self.generate_cursor(childs[-1], [""])

        return self.generator.generate_function_decl(rettyp, name, argsexpr, stmtexpr)

    def onRETURN_STMT(self, c, childs, childrepr, tokens):
        return self.generator.generate_return_stmt(childrepr)

    def onUNEXPOSED_EXPR(self, c, childs, childrepr, tokens):
        def remove_spaces(s):
            return "".join(s.split())

        if len(childs)==0:
            #print("**NO CHILDS IN onUNEXPOSED_EXPR:", c.spelling, "".join(tokens))
            return "" #FIXME: why no childs?

        child0expr = remove_spaces(self.generate_cursor_clean(childs[0]))
        fulltokens = concat_tokens(tokens)
        if len(fulltokens) > len(child0expr) and fulltokens.startswith(child0expr):
            # handles special case for some member access not reaching last element in structure depth access
            return fulltokens
        return self.generator.generate_unexposed_expr(childrepr, c.type.spelling)

    def onCONDITIONAL_OPERATOR(self, c, childs, childrepr, tokens):
        then_expr = self.generate_cursor(childs[1], ["then"])
        else_expr = self.generate_cursor(childs[2], ["else"])
        cond = self.generate_cursor(childs[0], ["cond"]) #this need to be calculated after the other expressions
        return self.generator.generate_conditional_operator(cond, then_expr, else_expr)

    def onSWITCH_STMT(self, c, childs, childrepr, tokens):
        switchvar = self.generate_cursor(childs[0], ["switchvar"])
        caselist = self.internal_generate_cursor_stmt(childs[1], ["caselist"])
        return self.generator.generate_switch(switchvar, caselist)

    def onCASE_STMT(self, c, childs, childrepr, tokens):
        caselabel = self.internal_generate_cursor_stmt(childs[0], ["caselabel"])
        casestmt = self.internal_generate_cursor_stmt(childs[1], ["casestmt"])
        return self.generator.generate_case_label(caselabel, casestmt)

    def onDEFAULT_STMT(self, c, childs, childrepr, tokens):
        casestmt = self.internal_generate_cursor_stmt(childs[0], ["casestmt"])
        return self.generator.generate_case_label(None, casestmt)

    def onFOR_STMT(self, c, childs, childrepr, tokens):
        assert(concat_tokens(tokens[:2]) == "for(")
        tokens = tokens[2:] #skips "for("
        l = [""]
        childn = 0
        d = [None, None, None]
        for t in tokens:
          if t.spelling == ";":
            l += [""]
          else:
            l[-1] = l[-1] + t.spelling
            ch = childs[childn]
            t = concat_tokens(ch.get_tokens())
            if l[-1] == t:
              d[len(l)-1] = t
              if len(l) >= len(d):
                break
              childn = childn + 1
        forbody = self.generate_cursor(childs[-1], ["forbody"])
        return self.generator.generate_for(d[0], d[1], d[2], forbody)

    def internal_generate_cursor_stmt(self, c, k0):
        if c.kind == CursorKind.COMPOUND_STMT:
            return self.generate_cursor(c, k0)
        self.generator.indent()
        s = self.generate_cursor(c, k0)
        self.generator.unindent()
        return s

    def onIF_STMT(self, c, childs, childrepr, tokens):
        then_stmt = self.internal_generate_cursor_stmt(childs[1], ["then"])
        else_stmt = (
            self.internal_generate_cursor_stmt(childs[2], ["else"]) if len(childs) > 2  else None
        )
        cond = self.generate_cursor(childs[0], ["cond"])
        return self.generator.generate_if_stmt(cond, then_stmt, else_stmt)

    def onDECL_REF_EXPR(self, c, childs, childrepr, tokens):
        name = c.spelling
        return self.generator.generate_decl_ref_expr(
            name
        )  # , childrepr childs seems not neccessary

    def onLITERAL(self, c, childs, childrepr, tokens):
        value = tokens[0].spelling
        typ = c.type.spelling
        return self.generator.generate_literal(value, typ)


    def onCXX_FUNCTIONAL_CAST_EXPR(self, c, childs, childrepr, tokens):
        if self.vector_overloaded_operators:
            def get_arg_typs(args):
                typs = []
                for argch in args:
                    typch, countch = self.get_canonicaltype_and_count(recurse_unexposed(argch))
                    typs += [typch]
                return "_".join(typs)

            typ, count = self.get_canonicaltype_and_count(c)
            if count:
                fname = typ + "_make_from_" + get_arg_typs(childs[1:])
                #FIXME: use childs_recurse(childs[1:])
                next(childrepr) #1st is TYPE_REF, 2nd is the expression
                return self.generator.generate_overloaded_call(fname, childrepr, c.typ.spelling) 
            
        target_type = c.type.spelling
        expr_type = childs[0].type.spelling
        return self.generator.generate_cxx_functional_castexpr(target_type, childrepr, expr_type) #1st TYPE_REF

    def onCSTYLE_CAST_EXPR(self, c, childs, childrepr, tokens):
        target_type = c.type.spelling
        expr_type = childs[0].type.spelling
        return self.generator.generate_cstyle_cast_expr(target_type, childrepr, expr_type) #1st TYPE_REF

    def onBINARY_OPERATOR(self, c, childs, childrepr, tokens):
        lhschild = childs[0]
        rhschild = childs[1]
        lhs = self.generate_cursor(lhschild, ["lhs"])
        rhs = self.generate_cursor(rhschild, ["rhs"])
        op = self.find_operator_before(c, rhschild)
        
        if op[-1] == "=" and op not in COMPARISON_OPERATORS: #all assignments
            #test for RHS being pointer
            r = recurse_unexposed(rhschild)
            if r.kind == CursorKind.UNARY_OPERATOR:
                c = [x for x in r.get_children()]
                if len(c) == 1:
                    r = recurse_unexposed(c[0])
                    if r.kind == CursorKind.DECL_REF_EXPR:
                        return self.generator.generate_pointer_read(lhs, self.generate_cursor(r))
            #test for LHS being pointer
            if lhschild.kind == CursorKind.UNARY_OPERATOR:
                c = [x for x in lhschild.get_children()]
                if len(c) == 1:
                    r = recurse_unexposed(c[0])
                    if r.kind == CursorKind.DECL_REF_EXPR:
                        return self.generator.generate_pointer_write(self.generate_cursor(r), rhs)
            return self.generator.generate_assignment_operator(lhs, op, rhs, c.type.spelling)

        try:
            opname = operator_to_name(op) #test existence
            pass
        except:
            print("unrecognized operator '"+op+"'", file=sys.stderr) #sometimes comes ")"
            pass
   

        if self.vector_overloaded_operators: #test vector types
            ltyp, lcount = self.get_canonicaltype_and_count(lhschild)
            rtyp, rcount = self.get_canonicaltype_and_count(rhschild)
            if lcount or rcount:
                #generate vector overloaded function call
                #print(ltyp, lhschild.spelling, op, rtyp, rhschild.spelling, file=sys.stderr)
                fname = bin_operator_to_name(ltyp, op, rtyp)
                return self.generator.generate_overloaded_call(fname, childrepr, c.type.spelling)
            elif ltyp in self.overloaded_types or rtyp in self.overloaded_types:
                #generate floating point overloaded function call
                #print(ltyp, lhschild.spelling, op, rtyp, rhschild.spelling, file=sys.stderr)
                fname = bin_operator_to_name(ltyp, op, rtyp)
                return self.generator.generate_overloaded_call(fname, childrepr, c.type.spelling)

        return self.generator.generate_binary_operator(lhs, op, rhs) #gets here if RHS has no-overloaded operators


    def onUNARY_OPERATOR(self, c, childs, childrepr, tokens):
        # negative literals that appears as unary operator
        if childs[0].kind in [CursorKind.INTEGER_LITERAL, CursorKind.FLOATING_LITERAL]:
            value = tokens[0].spelling + tokens[1].spelling #self.generate_cursor_clean(childs[0])
            typ = c.type.spelling
            return self.generator.generate_literal(value, typ)

        if tokens[0].spelling in UNARY_OPERATORS:
            op = tokens[0].spelling
            return self.generator.generate_unary_operator(op, childrepr)

        postfix = tokens[1].spelling
        return self.generator.generate_unary_operator_postfix(childrepr, postfix)

    def onDECL_STMT(self, c, childs, childrepr, tokens):
        # FIXME: seems to only join childs
        return self.generator.generate_decl_stmt(childrepr)

    def onVAR_DECL(self, c, childs, childrepr, tokens):
        decltyp = self.map_type(c)
        name = c.spelling
        has_value = not self.empty_childs(childs)
        rtyp = recurse_unexposed(childs[1]).type.spelling if len(childs)>1 else None
        valueexpr = childrepr
        storage = c.storage_class.name
        storage = None if storage == "NONE" else storage.lower()
        return self.generator.generate_var_decl(decltyp, name, has_value, valueexpr, rtyp, storage)

    def onTYPEDEF_DECL(self, c, childs, childrepr, tokens):
        decltyp = c.underlying_typedef_type
        name = c.spelling
        self.reverse_typedef_map[mangle_type(decltyp.spelling)] = c
        #print("typedef:", "'"+name+"'", "is", decltyp.spelling)
        
        if self.vector_overloaded_operators: #FIXME: avoid double declaration
            typ, count = self.get_canonicaltype_and_count(c)
            print
            if count:
                #s = "#warning replaced vector typedef " + name + " ("+typ+")"
                return self.generator.generate_typedef_decl(typ, name)
                
        return self.generator.generate_typedef_decl(decltyp.spelling, name)

    def map_type(self, c):
        return c.type.spelling

    def onPARM_DECL(self, c, childs, childrepr, tokens):
        decltyp = self.map_type(c)
        name = c.spelling
        return self.generator.generate_param_decl(decltyp, name)

    def onTYPE_REF(self, c, childs, childrepr, tokens):
        typname = c.type.spelling
        return self.generator.generate_type_ref(typname, childrepr)

    def onINIT_LIST_EXPR(self, c, childs, childrepr, tokens):
        return self.generator.generate_init_list(childrepr)

    def onNULL_STMT(self, c, childs, childrepr, tokens):
        return self.generator.generate_null_stmt()

    def onPAREN_EXPR(self, c, childs, childrepr, tokens):
        return self.generator.generate_paren_expr(childrepr)

    def onMEMBER_REF_EXPR(self, c, childs, childrepr, tokens):
        name = c.spelling
        if name.startswith("operator "):
            return self.generator.generate_expr(childrepr)
        parentyp = ""
        if len(childs):
          parentyp, _ = self.get_canonicaltype_and_count(childs[0])
        return self.generator.generate_member_ref_expr(childrepr, name, parentyp)

    def onUNION_DECL(self, c, childs, childrepr, tokens):
        return self.onSTRUCT_OR_UNION_DECL(c, childs, childrepr, tokens)

    def onSTRUCT_DECL(self, c, childs, childrepr, tokens):
        return self.onSTRUCT_OR_UNION_DECL(c, childs, childrepr, tokens)

    def onFIELD_DECL(self, c, childs, childrepr, tokens):
        fieldtyp = c.type.spelling
        name = c.spelling
        return self.generator.generate_field_decl(fieldtyp, name)

    def onSTRUCT_OR_UNION_DECL(self, c, childs, childrepr, tokens):
        keyword = tokens[0].spelling
        name = c.spelling
        fields = childrepr
        has_fields = not self.empty_childs(childs)
        return self.generator.generate_struct(keyword, name, fields, has_fields)

    def onCLASS_DECL(self, c, childs, childrepr, tokens):
        name = c.spelling
        members = childrepr
        return self.generator.generate_class(name, members)

    def print_diagnostics(self):
        sev = {2: "warning", 3: "ERROR", 4:"SEVERITY-4" }
        ok = True
        for diag in self.tu.diagnostics:
            print(
                self.tu.spelling + ":",
                sev[diag.severity],
                "line",
                str(diag.location.line) + ":" + str(diag.location.column),
                diag.spelling,
                file=sys.stderr,
            )
            if diag.severity > 2: ok = False
        return ok

    def generate(self, generator):
        self.generator = generator
        s = ""
        for k, c in enumerate(
            self.tu.cursor.get_children()
        ):  # want to skip root (traslation unit)
            s += self.generator.generate_stmt(self.generate_cursor(c, [str(k)]))
        return s


