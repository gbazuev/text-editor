This is a simple text editor running in the terminal. The project is based on a series of [Build Your Own Text Editor](https://viewsourcecode.org/snaptoken/kilo/) articles, which, in turn, are based on the [antirez's Kilo](http://antirez.com/news/108) project!

To build the editor, you need to do

    git clone https://github.com/gbazuev/text-editor/tree/master
    cd text-editor/
    make
## Editor launch
To create an empty file, run

    ./editor

To open an existing file, run

    ./editor <FILENAME>

## Search
To start searching for file content, press CTRL-F and start typing what you need. If there are several matches, then you can move between them using the arrows

## Highlighting
The editor is made in the Darcula color theme, because I really like it!

## How to exit?
Knowing how many people are imprisoned by Vim and its descendants right now, I leave this reminder here: to exit, press CTRL-Q
