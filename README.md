# No Delete
> Suhas N | 2023

## Description
This program injects into `explorer.exe` and plays with its Shift+Delete functionality.
You will know where to use this program.

## Usage
I recommend you to create a seperate directory.
1. Grab the `NoDelete.exe` file and the `NoDeleteH.dll` files from `Releases` and place them in the same folder.
2. Start `NoDelete.exe` program. It will download `shell32.pdb` (10-11MB, speed depends on MS symbol servers)
3. If the program ends, check the logs in the `log` folder.
4. The logs in the log folder as named as follows:
	+ `base.log`: contains information of the main executable
	+ `inject_{PID}.log`: contains information of the target process.

## Heads up
Yet implement the `auto-injection` feature. This will be implemented on demand.

## Disclaimer
I am not responsible for any damage caused to you by using this source code.
Don't be scared if `explorer.exe` and report the issue in #Issues with your logs.