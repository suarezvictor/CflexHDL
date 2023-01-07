#!/usr/bin/python3
"""
Parser/Generator to convert C files to Silice files (.ice)
(C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>

"""

import sys
sys.path.append("../../cflexparser")

from clangparser import CFlexClangParser, CFlexBasicCPPGenerator, concat_tokens, mangle_type, remove_type_qualifiers
from clang.cindex import TypeKind

def has_type_qualifier(typ, qualifier): #FIXME: move to clangparser
  #print("//type", typ, "qualifier", qualifier)
  return qualifier in typ

class CFlexSiliceGenerator(CFlexBasicCPPGenerator):
    def adjust_noncompund_statement(self, stmt):
        if not stmt or stmt.lstrip()[0] != "{":
            t = self.indent()
            stmt = "\n" + t + "{ " + stmt.lstrip() + " }" #surround single statements
            self.unindent()
        return stmt

    def generate_class(self, name, members):
        return ""

    def generate_typedef_decl(self, decltyp, name):
        pass

    def generate_struct(self, keyword, name, fields, has_fields):
        if keyword == "union":
            return next(fields) #emit just the first field
        if not has_fields:
            return ""

        return "group " + name + " { " + self.generate_expr(fields, " ") + " };"

    def generate_field_decl(self, fieldtyp, name):
        return "\n" + self.ind + fieldtyp + " " + name + ";"

    def generate_bitfield_decl(self, fieldtyp, name, width, offsetbits):
        return fieldtyp + " " + name + ":" + str(width) + " /*@" + str(offsetbits) + "*/;"

    def generate_decl_ref_expr(self, name):
        return name

    def generate_stmt(self, stmt):
        return self.ind + self.generate_expr(stmt)

    def generate_function_decl(self, rettyp, name, argsexpr, stmtexpr):
        if name == "__silice_main": name = "main" #avoid name clash with C++
        s = "(" + self.generate_expr(argsexpr, ",") + "\n)"
        print("...PROCESSING", name, s, file=sys.stderr)

        if stmtexpr is not None:
            if name[0] != '_':
              s += " <autorun>"
            else:
              name = name[1:]
            s += self.generate_expr(stmtexpr)

        return "algorithm " + name + s + "\n"

    def generate_param_decl(self, decltyp, name):
        t = self.indent()
        register = has_type_qualifier(decltyp, "register")
        if decltyp.endswith("&") and not decltyp.startswith("const"):
            decltyp = "output" + ("!" if register else "") + "\t" + remove_type_qualifiers(decltyp[:-2])
        else:
            decltyp = "input\t" + remove_type_qualifiers(decltyp)
        s = "\n"+t + decltyp + "\t" + name
        self.unindent()
        return s

    def generate_compound_stmt(self, stmtexpr):
        t = self.indent()
        s = self.generate_expr(stmtexpr)
        t = self.unindent()
        return "\n" + t + "{" + s +"\n"+t + "}"

    def generate_assignment_operator(self, lhs, op, rhs):
        return "\n" + self.ind + lhs + " " + op + " " + rhs + ";" # op: =, &=, >>=, etc

    #FIXME: move to generic class
    def generate_while(self, cond, expr):
        stmt = "while(" + cond + ")" #if cond != "1" else "always"
        return "\n" + self.ind + stmt + self.adjust_noncompund_statement(self.generate_expr(expr))

    def generate_bindarg(self, a, b, isinput):
        return a + ("\t<:\t" if isinput else "\t:>\t") + b

    def generate_module_instance(self, decltyp, name, instanceargs):
        s = "\n" + self.ind + decltyp + " " + name
        t = self.indent()
        bindings = t + (",\n"+t).join(instanceargs)
        self.unindent()
        return s + "(\n" + bindings + "\n" + self.ind + ");"

    def generate_var_decl(self, decltyp, name, has_value, valueexpr):
        s = "\n" + self.ind + decltyp + " " + name
        value =  self.generate_expr(valueexpr) if has_value else "uninitialized"
        return s + " = " + value + ";"

    def generate_call_instance(self, name, childrepr):
        return "\n" + self.ind + name + " <- " + self.generate_call("", childrepr)+";"

    def generate_if_stmt(self, cond, then_stmt, else_stmt):
        s = "\n" + self.ind + "if(" + cond + ")" + self.adjust_noncompund_statement(then_stmt)
        if else_stmt is None:
            return s
        s += "\n" + self.ind + "else"
        return s + self.adjust_noncompund_statement(else_stmt)

    def generate_null_stmt(self):
        return ""
        #return "\n++:" #insert clock statement FIXME: check yield() statement/macro usage

    def generate_comment(self, kind, comment):
        #return "#"+kind+"#"+comment
        return ""

    def generate_case_label(self, caselabel, casestmt):
        caselabel = "case " + caselabel if caselabel is not None else "default"
        return "\n" + self.ind + caselabel + ": " + casestmt #no "break"

    def generate_for(self, for1, for2, for3, forbody):
        #transform for in while
        s = "\n" + self.ind
        if for1 is not None:
          s += for1 + ";\n" + self.ind
        s += "while(" + ("1" if for2 is None else for2) + ")"
        if for3 is None:
          s += forbody
        else:
          s += " { //for" + forbody + " " + for3 + ";}"
        return s

from clang.cindex import CursorKind

def is_silice_native(typname):
    typname = remove_type_qualifiers(typname)
    if not typname.startswith("uint"):
        return True
    try:
        nbits = typname[4:]
        return (typname[:4] + str(int(nbits))) == typname
    except:
        return False

class CFlexClangParserSilice(CFlexClangParser):
    def __init__(self, file, args):
        super().__init__(file, args)
        self.overloaded_operators = False
        #self.no_parse_cursors = True

    def map_type(self, c):
        while True:
            if is_silice_native(c.type.spelling):
                decltyp = c.type.spelling
                break
            
            if c.type.kind == TypeKind.LVALUEREFERENCE:
                typedeftyp = c.type.get_pointee().get_declaration()
                ref = " &"
            else:
                ref = ""
                typedeftyp = c.type.get_declaration()

            if True:
                if is_silice_native(typedeftyp.underlying_typedef_type.spelling):
                    decltyp = typedeftyp.underlying_typedef_type.spelling + ref
                    break
                c = typedeftyp
            #except:
            #    decltyp = "AA"+c.type.spelling
            #    break
        return decltyp

    def unwind_type(cbase):
        def cond(typname):
            return False

        c = recurse_unexposed(cbase)
        ctyp = c.type.get_canonical()
        if not allowtemplate:
            ctypr = self.first_noncond_typedef(ctyp.spelling, cond)
            if ctypr is not None:
                ctyp = ctypr

        typname = ctyp.spelling
        return typname

    def onANY_KIND(self, c, childs, childrepr, tokens):
        if False:
            pass
        elif c.kind == CursorKind.WHILE_STMT:
            return self.onWHILE_STMT(c, childs, childrepr, tokens)

        return super().onANY_KIND(c, childs, childrepr, tokens)

    def onWHILE_STMT(self, c, childs, childrepr, tokens):
        cond = next(childrepr)  
        return self.generator.generate_while(cond, childrepr)

    def onFIELD_DECL(self, c, childs, childrepr, tokens):
        if not c.is_bitfield():
            if c.semantic_parent.kind != CursorKind.UNION_DECL:
                return super().onFIELD_DECL(c, childs, childrepr, tokens)
            fieldtyp = self.map_type(c)
            name = c.spelling
            return self.generator.generate_field_decl(fieldtyp, name + " = uninitialized")

        width = c.get_bitfield_width()
        offsetbits = c.get_field_offsetof()
        fieldtyp = c.type.spelling
        name = c.spelling
        return self.generator.generate_bitfield_decl(fieldtyp, name, width, offsetbits)

    def onMEMBER_REF_EXPR(self, c, childs, childrepr, tokens):
        def findchilds(childs, child):
            return [ch for ch in childs if ch.spelling == child.spelling]
        #if not childs: #probably class
        #    return super().onMEMBER_REF_EXPR(c, childs, childrepr, tokens)

        decl = childs[0].type.get_declaration()

        declch = decl.get_children()
        ch = findchilds(declch, c)
        if ch and ch[0].is_bitfield():
            ch = ch[0]
            offsetbits = ch.get_field_offsetof()
            width = ch.get_bitfield_width()
            uniondecl = ch.semantic_parent.semantic_parent
            if uniondecl.kind == CursorKind.UNION_DECL:
                ch0 = next(uniondecl.get_children()) #first brother
                return ch0.spelling + "["+str(offsetbits)+","+str(width)+"]"

        return super().onMEMBER_REF_EXPR(c, childs, childrepr, tokens)

    def onCALL_EXPR(self, c, childs, childrepr, tokens):
        # if len(childs) == 0: #no real calls
        #    return None
        name = c.spelling
        if name == "__sync_synchronize":
            return "\n++:"
        if name == "operator()":
            name = next(childrepr)
            next(childrepr)
            return self.generator.generate_call_instance(name, childrepr)
        return self.generator.generate_call(name, childrepr)

    def onVAR_DECL(self, c, childs, childrepr, tokens):
#CursorKind.CONSTRUCTOR#	//10.4[7] CursorKind.CONSTRUCTOR void (uint_div_width &, const uint16 &)  line 40:2 "div16" div16/(/uint_div_width/&/r/,/const/uint16/&/k/)/:/ret/(/r/)
#CursorKind.PARM_DECL#	//10.4.0[1] CursorKind.PARM_DECL uint_div_width &  line 40:24 "r" uint_div_width/&/r
#CursorKind.TYPE_REF#	//10.4.0.0[0] CursorKind.TYPE_REF uint_div_width  line 40:8 "uint_div_width" uint_div_width
#CursorKind.PARM_DECL#	//10.4.1[1] CursorKind.PARM_DECL const uint16 &  line 40:41 "k" const/uint16/&/k
#CursorKind.TYPE_REF#	//10.4.1.0[0] CursorKind.TYPE_REF uint16  line 40:33 "uint16" uint16
#CursorKind.MEMBER_REF#	//10.4.2[0] CursorKind.MEMBER_REF uint_div_width &  line 40:46 "ret" ret
#CursorKind.DECL_REF_EXPR#	//10.4.3[0] CursorKind.DECL_REF_EXPR uint_div_width  line 40:50 "r" r
#CursorKind.MEMBER_REF#	//10.4.4[0] CursorKind.MEMBER_REF uint16  line 40:54 "_k" _k
#CursorKind.UNEXPOSED_EXPR#	//10.4.5[1] CursorKind.UNEXPOSED_EXPR uint16  line 40:57 "k" k
#CursorKind.DECL_REF_EXPR#	//10.4.5.0[0] CursorKind.DECL_REF_EXPR const uint16  line 40:57 "k" k
#CursorKind.COMPOUND_STMT#	//10.4.6[0] CursorKind.COMPOUND_STMT   line 40:60 "" {/}
        """
          div$div_width$ div(
            ret :> inv_y
          );
        """
#CursorKind.DECL_STMT#	//12.7.11[1] CursorKind.DECL_STMT   line 103:3 "" div16/div/(/inv_y/,/myk/)/;
#CursorKind.VAR_DECL#	//12.7.11.0[2] CursorKind.VAR_DECL div16  line 103:9 "div" div16/div/(/inv_y/,/myk/)
#CursorKind.TYPE_REF#	//12.7.11.0.0[0] CursorKind.TYPE_REF div16  line 103:3 "class div16" div16
#CursorKind.CALL_EXPR#	//12.7.11.0.1[2] CursorKind.CALL_EXPR div16  line 103:9 "div16" div/(/inv_y/,/myk/)
#CursorKind.DECL_REF_EXPR#	//12.7.11.0.1.0[0] CursorKind.DECL_REF_EXPR uint_div_width  line 103:13 "inv_y" inv_y
#CursorKind.UNEXPOSED_EXPR#	//12.7.11.0.1.1[1] CursorKind.UNEXPOSED_EXPR const uint16  line 103:20 "myk" myk
#CursorKind.DECL_REF_EXPR#	//12.7.11.0.1.1.0[0] CursorKind.DECL_REF_EXPR uint16  line 103:20 "myk" myk

        decltyp = self.map_type(c)
        name = c.spelling
        if childs[0].spelling.startswith("class ") and childs[1].kind == CursorKind.CALL_EXPR:
            #FIXME: check if base is silice_module
            callcursor = childs[1]
            args = callcursor.get_children()
            classdecl = childs[0].type.get_declaration()
            constructors = [c for c in classdecl.get_children() if c.kind == CursorKind.CONSTRUCTOR]
            argnames = [a for a in constructors[0].get_children() if a.kind == CursorKind.PARM_DECL]
            callargs = [a for a in callcursor.get_children()] 
            instanceargs = [self.generator.generate_bindarg(a.spelling, b.spelling, a.type.spelling.startswith("const")) for a, b in zip(argnames, callargs)]
            return self.generator.generate_module_instance(decltyp, name, instanceargs)
        has_value = not self.empty_childs(childs)
        valueexpr = childrepr
        return self.generator.generate_var_decl(decltyp, name, has_value, valueexpr)


#USAGE: silice_generator.py source.c [CFLAGS]

if __name__ == "__main__":
    import sys
    sourcefile = sys.argv[1]
    cflags = sys.argv[2:]
    parser = CFlexClangParserSilice(sourcefile, cflags)
    parser.print_diagnostics()
    generated = parser.generate(CFlexSiliceGenerator())
    print(generated)

