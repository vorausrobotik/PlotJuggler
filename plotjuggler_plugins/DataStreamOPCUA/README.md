# OPC UA (open62541) plugin

This plugin allows you to connect PlotJuggler to an [open62541](https://open62541.org/) server which
is a free implementation of the OPC UA protocol.

## External dependencies

 - Open62541 must be installed. It can be cloned form its repo [open62541](https://github.com/open62541/open62541). Instructions for manual installation can be found on [Installing open62541](https://open62541.org/doc/1.1/installing.html). After installation, it is recommended to update the links cache to shared libraries using `sudo ldconfig`.
 

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
- [Read Service](https://reference.opcfoundation.org/Core/Part4/v104/docs/5.10.2) is used instead of [Subscriptions](https://reference.opcfoundation.org/Core/Part4/v104/docs/5.12)
