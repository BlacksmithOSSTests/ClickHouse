import os
import sys

from ci.praktika.gh import GH
from ci.praktika.info import Info

integrations_ecosystem_files = ["src/Core/TypeId.h"]

# Skip if running on blacksmith runners (no AWS/DB)
if os.environ.get("AWS_EC2_METADATA_DISABLED") == "true":
    print("[team_notifications.py] Skipping: AWS_EC2_METADATA_DISABLED is set.")
    exit(0)

def check():
    info = Info()

    changed_files = info.get_custom_data("changed_files")
    for file in changed_files:
        if any(file.startswith(f) for f in integrations_ecosystem_files):
            GH.post_updateable_comment(
                comment_tags_and_bodies={
                    "team_notigication": "@ClickHouse/integrations-ecosystem please, take a look"
                }
            )
            break

    return True


if __name__ == "__main__":
    if not check():
        sys.exit(1)
