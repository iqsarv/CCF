#!/bin/bash
# Copyright (c) Microsoft Corporation. All rights reserved.
# Licensed under the Apache 2.0 License.

set -e

if [ -z "$1" ]; then
    echo "The application enclave file should be specified (e.g. liblogging)"
    exit 1
fi

PACKAGE="$1"
shift

echo "Setting up Python environment..."
if [ ! -f "env/bin/activate" ]
    then
        python3.7 -m venv env
fi
source env/bin/activate
pip install -q -U -r ../tests/requirements.txt
echo "Python environment successfully setup"

CURL_CLIENT=ON python ../tests/start_network.py --package "${PACKAGE}" --label test_network "$@"