#!/bin/bash

# Set capabilities for the executable

sudo setcap cap_net_raw=eip build/ecap
