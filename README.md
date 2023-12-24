**cmenu** 
<br> a minimal tui menu thing

https://github.com/10xJSChad/cmenu/assets/48174610/056e0959-8b16-4b4c-91c6-ad8dd161197a


```cmenu``` reads newline separated entries from stdin and creates a simple selection menu. Search entries by typing, use up/down arrows to navigate, hit enter to select.

The entry is written to the output file specified in the first argument, an optional second argument of ```-r``` or ```--remove``` tells cmenu to print the contents of the file to stdout, and then delete it. This can be used to create dmenu-like scripts for the terminal.

An example script is provided in this repository, the script covers all you need to know, as ```cmenu``` has no additional features other than what you see in the example script.

Limitations:
* The amount of entries and the length of each entry is limited, although the limit is way beyond what's required for the vast majority of use cases I can think of. Nevertheless, it can be increased by changing the ```ENTRY_MAX``` and ```ENTRIES_MAX``` constants in cmenu.c.

* The pattern matching is very barebones, it matches from the start of each entry, so "orang" will match "orange", but "rang" will not.

```
usage:
    cmenu <OUTPUT_FILE> [-r|--remove]

building:
    cc cmenu.c
```
