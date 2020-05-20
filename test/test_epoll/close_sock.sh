#!/usr/bin/env bash
kill -9 $(lsof -i -P -n | grep "1234" | cut -d " " -f 2)