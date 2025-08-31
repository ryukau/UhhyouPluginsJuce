import shutil
from pathlib import Path


def deployVst3():
    vst3_dir = Path("C:/Program Files/Common Files/VST3")
    for category_dir in ["experimental", "plugin"]:
        artefact_dir = Path("build") / category_dir
        for name in [p.name for p in Path(category_dir).glob("*/")]:
            src = artefact_dir / Path(
                f"{name}/{name}_artefacts/Release/VST3/{name}.vst3"
            )
            dst = vst3_dir / Path(name)

            try:
                shutil.copytree(src, dst, dirs_exist_ok=True)
            except FileNotFoundError:
                print(f"{name} have not been built.")


if __name__ == "__main__":
    deployVst3()
