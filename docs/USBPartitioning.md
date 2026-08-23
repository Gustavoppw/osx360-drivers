## USB Partition Layout
The USB MUST USE A **MBR** PARTITION TABLE or it wont be recognized by the console, this is especially necessary for using bad update (or abadavatar)

### RGH / JTAG

For a console with RGH/JTAG:

USB (MBR)
├── Partition 1 — FAT32
│   ├── XeLL Reloaded
│   ├── XeLL Launcher
│   └── OpenBIOS
│
└── Partition 2 — HFS+
    └── Mac OS X Tiger 10.4 installer[¹](#some-details)
    
### Badupdate/abadavatar

USB (MBR)
├── Partition 1 — FAT32
│   ├── BadUpdate[²](#some-details)
│   ├── XeLL Reloaded
│   ├── XeLL Launcher
│   └── OpenBIOS
│
└── Partition 2 — HFS+
    └── Mac OS X Tiger 10.4 installer[¹](#some-details)


## Some details
¹ Some images might not work, the one i tried i found on the internet archive
² a bad avatar *might* if you are sure you're going to install to an external drive, as i had issues with abadavatar without a hard drive that the Xbox 360 could see, even if openbios/OSX Tiger did
