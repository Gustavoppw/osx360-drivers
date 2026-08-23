## USB Partition Layout
The USB MUST USE A **MBR** PARTITION TABLE or it wont be recognized by the console, this is especially necessary for using bad update (or Abadavatar)

### RGH / JTAG

For a console with RGH/JTAG:

```text
USB (MBR)
├── Partition 1 — FAT32
│   ├── XeLL Reloaded
│   ├── XeLL Launcher
│   └── OpenBIOS
│
└── Partition 2 — HFS+
    └── Mac OS X Tiger 10.4 installer¹
```
    
### Badupdate/Abadavatar


For a console with Badupdate/Abadavatar

```text
USB (MBR)
├── Partition 1 — FAT32
│   ├── BadUpdate²
│   ├── XeLL Reloaded
│   ├── XeLL Launcher
│   └── OpenBIOS
│
└── Partition 2 — HFS+
    └── Mac OS X Tiger 10.4 installer¹
```

## Some details
¹ Some images might not work, the one i tried i found on the internet archive

² Abadavatar *might* if you are sure you're going to install to an external drive, as i had issues with Abadavatar without a hard drive that the Xbox 360 could see, even if openbios/OSX Tiger did
