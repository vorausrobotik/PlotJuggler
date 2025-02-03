# OPC UA (open62541) plugin

This plugin allows you to connect PlotJuggler to an [open62541](https://open62541.org/) server which
is a free implementation of the OPC UA protocol.

## External dependencies

 - [open62541](https://github.com/open62541/open62541)
 - [jsoncons](https://github.com/danielaparker/jsoncons)

## How to build (cmake)

```shell script
git clone https://github.com/facontidavide/PlotJuggler.git
cd PlotJuggler
git submodule update --init --recursive
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DWITH_OPCUA=ON ..
```

## How to use

- Open PlotJuggler and select the `Start: OPCUA Stream` under the `Streaming` dropdown menu.
- You can add as many (supported) OPC UA variables in the dialog
- If you want, you can save your configuration to a json file by clicking the `Save to JSON` button
- This would be totally useless without the import functionality, right? Right. Click the `Load from JSON` button in
  to load your saved configuration the file again
- Click `OK` when you're done adding your variables. Your variables will be shown in the variable tree immediately 

## Limitations

- Only numeric values (and arrays) are are supported at the moment
- Update interval can't be defined per-value
