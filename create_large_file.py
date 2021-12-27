#! /usr/bin/env/ python3

with open("1gb.txt", "wb") as out:
    out.write(b"c" * 1000000)
