#! /usr/bin/env python3

import os
import os.path
import sys
import uritemplate

sys.path.append("adabot")
import github_requests as github

exit_status = 0

for dirpath, dirnames, filenames in os.walk("../bin"):
    if not filenames:
        continue
    for filename in filenames:
        full_filename = os.path.join(dirpath, filename)
        label = filename.replace("adafruit-circuitpython-", "")
        url_vars = {}
        url_vars["name"] = filename
        url_vars["label"] = label
        url = uritemplate.expand(os.environ["UPLOAD_URL"], url_vars)
        headers = {"content-type": "application/octet-stream"}
        print(url)
        with open(full_filename, "rb") as f:
            response = github.post(url, data=f, headers=headers)
        if not response.ok:
            if response.status_code == 422 and response.json().get("errors", [{"code":""}])[0]["code"] == "already_exists":
                print("File already uploaded. Skipping.")
                continue
            print("Upload of {} failed with {}.".format(filename, response.status_code))
            print(response.text)
            sys.exit(response.status_code)

sys.exit(exit_status)
