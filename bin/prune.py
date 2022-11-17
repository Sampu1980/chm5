from pathlib import Path
import os
import copy
import shutil

folder = "out/"
for files in os.listdir(folder):
    shutil.rmtree(folder + files)


ywrapper_path = "github.com/openconfig/ygot/proto/ywrapper/ywrapper.proto"
ywrapper_ns = "ywrapper."
wrapper_path = "google/protobuf/wrappers.proto"
wrapper_ns = "google.protobuf."

datatypes = ["BytesValue", "BoolValue", "Decimal64Value", "IntValue", "StringValue", "UintValue"]


for in_path in Path('.').rglob('*.proto'):
    in_path_str = copy.deepcopy(str(in_path))
    in_path = str(in_path)
    out_path = in_path_str.replace("in", 'out', 1)
    #print(in_path, out_path)
    if not os.path.exists(os.path.dirname(out_path)):
        os.makedirs(os.path.dirname(out_path))
    fin = open(in_path, "rt")
    fout = open(out_path, "wt")

    enum_found = False
    num = 1
    for line in fin:
        write_done = False
        #print("%d: %s" % (num, line))
        if line.find(ywrapper_path) != -1:
            #print("Replacing import path")
            fout.write(line.replace(ywrapper_path, wrapper_path))
            write_done = True
        for datatype in datatypes:
            if line.find(ywrapper_ns + datatype) != -1:
                #print ("Replacing datatype: %s" % datatype)
                if datatype == "UintValue":
                    gdatatype = "UInt32Value"
                elif datatype == "IntValue":
                    gdatatype = "Int32Value"
                elif datatype == "Decimal64Value":
                    gdatatype = "DoubleValue"
                else:
                    gdatatype = datatype
                fout.write(line.replace(ywrapper_ns+datatype, wrapper_ns+gdatatype))
                write_done = True
        #skip enum.proto for now
        if line.find("enum") != -1:
            enum_found = True
            var1 = line.split()
            enumVar = var1[1]
            #print("enumVar %s" % enumVar)
            enumVar = enumVar.upper()
            enumVar = enumVar + "_"
            #print("To upper %s" % enumVar)
        elif enum_found == True and line.find(enumVar) != -1:
            if line.find("UNSET") == -1 and in_path.find("enum") == -1:
                #print("Replace:%s" % enumVar)
                fout.write(line.replace(enumVar,"",1))
                write_done = True
        elif enum_found == True and line.find("}") != -1:
            enum_found = False
#            print("found end enum %s" % line)

        if not write_done:
            fout.write(line)

        num = num+1

    fin.close()
    fout.close()
