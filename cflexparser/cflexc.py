# (C) 2021 Victor Suarez Rovere <suarezvictor@gmail.com>

#NOTES:
"""
#test command:
$ 
$ clang -E -I. ../tr_pipelinec.cpp > tr_pipelinec.E.cpp && python3 cflexc.py tr_pipelinec.E.cpp > tr_pipelinec.gen.cpp && clang -c tr_pipelinec.gen.cpp -o tr_pipelinec.gen.o && clang++ -O3 -I.. -fopenmp=libiomp5 -ffast-math `sdl2-config --cflags --libs` main.cpp -o tr  && ./tr

or for C:

$ clang -E -I. ../tr_pipelinec.cpp > tr_pipelinec.E.cpp && python3 cflexc.py tr_pipelinec.E.cpp > tr_pipelinec.gen.c && clang -c tr_pipelinec.gen.c -include "c_compat.h" -o tr_pipelinec.gen.o && clang++ -O3 -I.. -fopenmp=libiomp5 -ffast-math `sdl2-config --cflags --libs` main.c -o tr  && ./tr

"""
from clangparser import CFlexClangParser, CFlexBasicCPPGenerator


class CFlexCGenerator(CFlexBasicCPPGenerator):
    # surrounds cast for c-style casting
    def generate_cxx_functional_castexpr(self, target_type, expr, expr_type):
        #all C++ style casts to C style
        return self.generate_cstyle_cast_expr(target_type, expr, expr_type)


    def generate_cstyle_cast_expr(self, target_type, expr, expr_type):
        if target_type == expr_type: #same cast: not needed
          return self.generate_expr(expr)
        return super().generate_cstyle_cast_expr(target_type, expr, expr_type)

    def generate_struct(self, keyword, name, fields, hasfields):
        s = "typedef " if name and hasfields else ""
        s += keyword + " " + name  # keyword = struct or union
        if hasfields:
            s += " { " + self.generate_expr(fields) + "} " + name + ";"
        return s #+ ";\n"

    def generate_class(self, name, members):
        return "//CLASS " + name +" (classes not supported)"

    def generate_typedef_decl(self, decltyp, name):
        if "<" in decltyp: return "//typedef does not support for template types: " + decltyp
        return super().generate_typedef_decl(decltyp, name)

    #TODO: add "in/inout", etc

#TODO: be smarter to avoid confusing types like "int_hello_t"
#and make a general function for that
def is_numeric_type(typ): 
	return len(typ)>=5 and typ[-2:]=="_t" and (typ[:3]=="int" or typ[:4]=="uint")

class CFlexPipelineCGenerator(CFlexCGenerator):
    def generate_typedef_decl(self, decltyp, name):
        if is_numeric_type(name):
          return "" #numerics are intrisic #super().generate_typedef_decl(decltyp, name)
        if "<" in decltyp: return "//typedef does not support for template types: " + decltyp
        return "#define " + name + " " + decltyp #replace typedef by macro

    def generate_function_decl(self, rettyp, name, argsexpr, stmtexpr):
        s = super().generate_function_decl(rettyp, name, argsexpr, stmtexpr)
        if not stmtexpr: #skip prototypes
            return "//" + s
        return s

    def generate_var_decl(self, decltyp, name, has_value, valueexpr, storage):
        if storage=="static" and decltyp.startswith("const "): #constants are translated to macros
          value = self.generate_expr(valueexpr)
          if not value.startswith("{"): value = "(" + value + ")" #surround, except initializer lists
          return "#define " + name + "\t" + value
        s = super().generate_var_decl(decltyp, name, has_value, valueexpr, storage)
        if storage: #skip all storage types different from ""
          return "//" + s
        return s

    def generate_binary_operator(self, lhs, op, rhs): #replace &&, || by &, |
        if op in ["||", "&&"]:
        	lhs = "((" + lhs + ")!=0)"
        	rhs = "((" + rhs + ")!=0)"
        	op = op[0]
        return lhs + " " + op + " " + rhs

    def generate_member_ref_expr(self, parentexpr, name, parenttyp):
        #this translates vector members ".r/.g/.b" to their alias ".x/.y/.z"
        paren = self.generate_expr(parentexpr)
        if not name: #maybe anonymous
          return paren
        if paren:
          paren += "."
          if parenttyp in ["fixed3", "float3"]:
            if name=="r": name="x"
            if name=="g": name="y"
            if name=="b": name="z"
        return paren + name

    def generate_literal(self, value, typ):
        if typ in ["float", "double"]:
          return "(" + typ + ")" + value #do a cast on all defined constants
        return super().generate_literal(value, typ)

import sys
if __name__ == "__main__":
    sourcefile = sys.argv[1]
    cflags = sys.argv[2:]
    parser = CFlexClangParser(sourcefile, cflags)
    parser.comments_enabled = False #comment line to enable comments
    parser.print_diagnostics()
    generated = parser.generate(CFlexPipelineCGenerator())
    print(generated)
