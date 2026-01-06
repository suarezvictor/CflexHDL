# Basic C to C (identity) generator
# (C) 2022 Victor Suarez Rovere <suarezvictor@gmail.com>
import sys

class CFlexGenerator:
    def __init__(self, indentation="  "):
        self.ind = ""
        self.indentation = indentation

    def indent(self):
        self.ind += self.indentation
        return self.ind

    def unindent(self):
        self.ind = self.ind[:-len(self.indentation)]
        return self.ind

    def generate_any_expr(self, childs, joinexpr=""): #catch all generator
        return self.generate_expr(childs, joinexpr);

class CFlexBasicCPPGenerator(CFlexGenerator):
    def generate_expr(self, childs, joinexpr=""):
        return joinexpr.join(childs)

    def generate_comment(self, kind, comment):
        return comment

    def generate_struct(self, keyword, name, fields, has_fields):
        s = keyword + " " + name  # token = struct or union
        if not has_fields:
            return s + "; "
        return s + " { " + self.generate_expr(fields) + "};"

    def generate_class(self, name, members):
        return "class " + name + " { " + self.generate_expr(members) + "};"

    def generate_field_decl(self, fieldtyp, name):
        return fieldtyp + " " + name + "; "

    def generate_cxx_functional_castexpr(self, target_type, expr, expr_type):
        return target_type + "(" + self.generate_expr(expr) + ")"

    def generate_cstyle_cast_expr(self, target_type, expr, expr_type):
        return "(" + target_type + ")(" + self.generate_expr(expr) + ")"

    def generate_unary_operator(self, op, expr):
        return op + self.generate_expr(expr)

    def generate_unary_operator_postfix(self, expr, postfix):
        return self.generate_expr(expr) + postfix

    def generate_init_list(self, expr):
        return "{" + self.generate_expr(expr, ", ") + "}"

    def generate_member_ref_expr(self, parentexpr, name, parenttyp):
        paren = self.generate_expr(parentexpr)
        if paren: paren += "."
        return paren + name

    def generate_paren_expr(self, expr):
        return "(" + self.generate_expr(expr) + ")"

    def generate_null_stmt(self):
        return ";"

    def generate_param_decl(self, decltyp, name):
        return decltyp + " " + name

    def generate_typedef_decl(self, decltyp, name):
        return "typedef " + decltyp + " " + name + ";"

    def generate_var_decl(self, decltyp, name, has_value, valueexpr, storage):
        storage = storage + " " if storage else ""
        s = storage + decltyp + " " + name
        if not has_value:
            return s + ";"
        return s + " = " + self.generate_expr(valueexpr) + ";"

    def generate_literal(self, value, typ):
        return value

    def generate_binary_operator(self, lhs, op, rhs, ltyp):
        return lhs + " " + op + " " + rhs

    #def generate_overloaded_binary_operator(self, lhs, op, rhs):
    #    return "<" + lhs + " [overloaded]" + op + " " + rhs + ">"

    def generate_decl_ref_expr(self, name):
        return name

    def generate_stmt(self, stmt):
        return self.ind + stmt + "\n"

    def generate_if_stmt(self, cond, then_stmt, else_stmt):
        s = "\n" + self.ind + "if(" + cond + ") " + then_stmt
        if else_stmt is None:
            return s
        s += "\n" + self.ind + "else "
        return s + else_stmt

    def generate_conditional_operator(self, cond, then_expr, else_expr):
        return cond + " ? " + then_expr + " : " + else_expr

    def generate_return_stmt(self, expr):
        return "return " + self.generate_expr(expr) + ";"

    def generate_function_decl(self, rettyp, name, argsexpr, stmtexpr):
        s = rettyp + " " + name
        s += "(" + self.generate_expr(argsexpr, ", ") + ")"
        print(s+";", file=sys.stderr)

        if not stmtexpr: #None or empty
            s += ";"  # just a prototype
        else:
            s += "\n" + self.generate_expr(stmtexpr)
        return s + "\n"

    def generate_compound_stmt(self, stmtexpr):
        t = self.indent()
        s = t + self.generate_expr(stmtexpr, "\n"+t)
        t = self.unindent()
        return "{\n" + s + "\n" + t + "}"

    def generate_assignment_operator(self, lhs, op, rhs, ltyp):
        return lhs + " " + op + " " + rhs + ";" # op: =, &=, >>=, etc

    def generate_call(self, name, argsexpr):
        return name + "(" + self.generate_expr(argsexpr, ", ") + ")"

    def generate_unexposed_expr(self, expr):
        pass

    def generate_decl_stmt(self, expr):
        # FIXME: in "for", maybe multiple variables are defined and it should use one statement for each
        #return self.generate_expr(expr) #+ ";"
        pass

    def generate_type_ref(self, typname, expr):
        pass

    def generate_switch(self, switchvar, caselist):
        return "\n" + self.ind + "switch(" + switchvar + ")" + caselist

    def generate_case_label(self, caselabel, casestmt):
        caselabel = "case " + caselabel if caselabel is not None else "default"
        return "\n" + self.ind + caselabel + ": " + casestmt + "\n" + self.ind + "break;"
      
    def generate_for(self, for1, for2, for3, forbody):
        s = "for(" + for1 + ";" + for2 + ";" + for3 + ")"
        return s + forbody

    def generate_pointer_write(self, ptr, rhs):
        return "\n" + self.ind + "bus_write(" + ptr + ", " + rhs + ");\n"

    def generate_pointer_read(self, lhs, ptr):
        return "\n" + self.ind + "lhs = bus_read(" + rhs + ");\n"

