#!/usr/bin/python

import json
import csv
import sys
import os

class FancyFloat(float):
    def __repr__(self):
        return format(Decimal(self), "f")

class JsonRpcEncoder(json.JSONEncoder):
    def decimalize(self, val):
        if isinstance(val, dict):
            return {k:self.decimalize(v) for k,v in val.items()}

        if isinstance(val, (list, tuple)):
            return type(val)(self.decimalize(v) for v in val)

        if isinstance(val, float):
            return FancyFloat(val)

        return val

    def encode(self, val):
        return super().encode(self.decimalize(val))

if len(sys.argv) == 1:
    sys.exit(1)

trace_events = []
call_stack = []
l = [p for p in os.listdir(sys.argv[1]) if p.endswith(".report")]
if len(l) == 0:
    print("No reports are in this folder")
    sys.exit(1)

file_name = l[0]
with open(file_name, "r") as in_file:
    csv_reader = csv.reader(in_file, delimiter=',')
    pf = 1
    first_line = True
    last_ts = -1;
    for line in csv_reader:
        if first_line:
            pf = float(line[0]) / 1000
            first_line = False
            continue
        if line[0] == "->":
            call_stack.append(line)
        elif line[0] == "<-":
            call = call_stack.pop()
            ts = float(call[1])
            dur = (float(line[1])-ts)
            dict = {
                "pid": 1,
                "tid": 1,
                "ts" : ts,
                "dur": dur,
                "ph" : "X",
                "name": call[2],
                "args": {
                    "file":  f"({call[3]}:{call[4]})",
                }
            }
            if call[5]:
                dict["args"]["info1"] = call[5]
            if call[6]:
                dict["args"]["info2"] = call[6]
            trace_events.append(dict)
        else:
            print("invalid syntax")
            break
with open(f"{file_name}.json", "w") as out_file:
    out_file.write(json.dumps({"traceEvents": trace_events}, indent=4))
