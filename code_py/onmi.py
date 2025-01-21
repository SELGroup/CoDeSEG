# -*- coding: utf8 -*-
import os
import shutil
import subprocess

binary = "../OvpNMI/bin/Release/onmi"

tmp_dir = "../graph_dataset/result"


def overlapping_normalized_mutual_information(pred, target,dataname):
    nodes = set(n for c in target for n in c)
    node_idx = {n: i for i, n in enumerate(nodes)}

    pred = [[node_idx[n] for n in c] for c in pred]
    target = [[node_idx[n] for n in c] for c in target]

    shutil.rmtree(tmp_dir, ignore_errors=True)
    path = os.path.join(tmp_dir,dataname)
    os.makedirs(path, exist_ok=True)

    pred_path = os.path.join(path,"pred.cnl")
    with open(pred_path, 'w', encoding='utf8') as f:
        for c in pred:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()

    target_path = os.path.join(path,"target.cnl")
    with open(target_path, 'w', encoding='utf8') as f:
        for c in target:
            f.write(" ".join([str(n) for n in c]))
            f.write("\n")
        f.flush()

    cmd_onmi = [
        binary,
        pred_path,
        target_path
    ]

    out = subprocess.run(cmd_onmi, capture_output=True)
    if out.returncode != 0:
        raise RuntimeError(out.stderr)

    out = out.stdout.decode("utf-8")

    onmi = float(out.split("\n")[-2])
    return onmi


