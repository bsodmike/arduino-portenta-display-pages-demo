# arduino-portenta-display-pages-demo

This demo uses the Arduino GIGA Wifi R1 board.

## Flashing with `dfu-util` 

Use the correct device `id` for your board.

```
// run as root
cd ~/Arduino/arduino-portenta-display-pages-demo/sketch_display_demo/build/arduino.mbed_giga.giga
sudo dfu-util -a 0 -d 2341:0366 -s 0x08040000 -D *.bin
```

## License

See `LICENSE`