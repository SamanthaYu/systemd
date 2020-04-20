import argparse
import json

def get_include_dirs(include_dir_file):
    include_dirs = list()
    with open(include_dir_file) as f:
        for line in f:
            stripped_line = line.strip()
            include_dir = stripped_line.replace("'", "").replace("',", "")
            include_dirs.append(include_dir)
    return include_dirs

def get_compile_db(compile_json):
    with open(compile_json) as f:
        compile_db = json.loads(f.read())
    return compile_db

def filter_commands(include_dirs, compile_db):
    filtered_db = list()

    for entry in compile_db:
        for include_dir in include_dirs:
            if entry["file"].startswith(include_dir) or entry["file"].startswith("../" + include_dir):
                filtered_db.append(entry)
    return filtered_db

def write_json(filtered_db, output_path):
    with open(output_path, "w") as f:
        f.write(json.dumps(filtered_db, indent=4))

if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser(description="Filter compile_commands.json to only include relevant compilation entries")
    arg_parser.add_argument("--include_dir", required=True, help="Path to file containing list of include directories")
    arg_parser.add_argument("--compile_commands", required=True, help="Path to compilation database")
    arg_parser.add_argument("--output", required=True, help="Path to filtered compilation database")
    args = arg_parser.parse_args()

    include_dirs = get_include_dirs(args.include_dir)
    compile_db = get_compile_db(args.compile_commands)

    filtered_compile_db = filter_commands(include_dirs, compile_db)
    write_json(filtered_compile_db, args.output)
