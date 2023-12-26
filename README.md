**cmenu** 
<br> a minimal tui menu thing

https://github.com/10xJSChad/cmenu/assets/48174610/056e0959-8b16-4b4c-91c6-ad8dd161197a


```cmenu``` reads newline separated entries from stdin and creates a simple selection menu. Search entries by typing, use up/down arrows to navigate, hit enter to select.
The selected entry is printed to stdout.

An example script is provided in this repository, the script covers all you need to know, as ```cmenu``` has no additional features other than what you see in the example script.

Limitations:
* The pattern matching is very barebones, it matches from the start of each entry, so "orang" will match "orange", but "rang" will not. Additionally, it's ascii only.

```
usage:
    cmenu

building:
    cc cmenu.c
```
