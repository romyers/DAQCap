#!/bin/bash

# Set capabilities for the executable

# TODO: Windows, MacOS versions

sudo setcap cap_net_raw=eip build/ecap
