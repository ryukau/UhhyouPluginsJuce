import json
import numpy as np

with open("test.json", "r", encoding="utf-8") as fp:
    test = json.load(fp)

with open("target.json", "r", encoding="utf-8") as fp:
    target = json.load(fp)

for idx in range(len(test)):
    d0 = test[idx]
    d1 = target[idx]

    if d0["n"] != d1["n"]:
        print(f"Mismatched `n`, {idx}, {d0['n']}, {d1['n']}")
        exit()

    nt = min([d0["nt"], d1["nt"]])

    np.testing.assert_allclose(np.array(d0["rj0"][:nt]), np.array(d1["rj0"][:nt]))
    np.testing.assert_allclose(np.array(d0["rj1"][:nt]), np.array(d1["rj1"][:nt]))
    np.testing.assert_allclose(np.array(d0["ry0"][:nt]), np.array(d1["ry0"][:nt]))
    np.testing.assert_allclose(np.array(d0["ry1"][:nt]), np.array(d1["ry1"][:nt]))
