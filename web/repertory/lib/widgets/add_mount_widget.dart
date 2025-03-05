import 'package:flutter/material.dart';

class AddMountWidget extends StatefulWidget {
  final String mountType;
  final void Function(String? newApiAuth) onApiAuthChanged;
  final void Function(String? newApiPort) onApiPortChanged;
  final void Function(String? newBucket) onBucketChanged;
  final void Function(String? newEncryptionToken) onEncryptionTokenChanged;
  final void Function(String? newHostNameOrIp) onHostNameOrIpChanged;
  final void Function(String? newName) onNameChanged;
  final void Function(String? newPath) onPathChanged;
  final void Function(String? newType) onTypeChanged;

  const AddMountWidget({
    super.key,
    required this.mountType,
    required this.onApiAuthChanged,
    required this.onApiPortChanged,
    required this.onBucketChanged,
    required this.onEncryptionTokenChanged,
    required this.onHostNameOrIpChanged,
    required this.onNameChanged,
    required this.onPathChanged,
    required this.onTypeChanged,
  });

  @override
  State<AddMountWidget> createState() => _AddMountWidgetState();
}

class _AddMountWidgetState extends State<AddMountWidget> {
  static const _items = <String>['Encrypt', 'Remote', 'S3', 'Sia'];
  static const _padding = 10.0;

  String? _mountType;

  @override
  void initState() {
    _mountType = widget.mountType;
    super.initState();
  }

  List<Widget> _createTextField(String name, onChanged) {
    return [
      const SizedBox(height: _padding),
      Text(
        name,
        textAlign: TextAlign.left,
        style: TextStyle(
          color: Theme.of(context).colorScheme.onSurface,
          fontWeight: FontWeight.bold,
        ),
      ),
      TextField(decoration: InputDecoration(), onChanged: onChanged),
    ];
  }

  @override
  Widget build(BuildContext context) {
    final mountTypeLower = _mountType?.toLowerCase();
    return Column(
      mainAxisSize: MainAxisSize.min,
      mainAxisAlignment: MainAxisAlignment.start,
      crossAxisAlignment: CrossAxisAlignment.start,
      children: [
        Column(
          mainAxisAlignment: MainAxisAlignment.start,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              'Provider Type',
              textAlign: TextAlign.left,
              style: TextStyle(
                color: Theme.of(context).colorScheme.onSurface,
                fontWeight: FontWeight.bold,
              ),
            ),
            const SizedBox(width: _padding),
            DropdownButton<String>(
              value: _mountType,
              onChanged: (value) {
                setState(() {
                  _mountType = value;
                });
                widget.onTypeChanged(value);
              },
              items:
                  _items.map<DropdownMenuItem<String>>((item) {
                    return DropdownMenuItem<String>(
                      value: item,
                      child: Text(item),
                    );
                  }).toList(),
            ),
          ],
        ),
        ..._createTextField('Configuration Name', widget.onNameChanged),
        if (mountTypeLower == 'encrypt')
          ..._createTextField('Path', widget.onPathChanged),
        if (mountTypeLower == 'sia')
          ..._createTextField('ApiAuth', widget.onApiAuthChanged),
        if (mountTypeLower == 's3' || mountTypeLower == 'sia')
          ..._createTextField('Bucket', widget.onBucketChanged),
        if (mountTypeLower == 'remote')
          ..._createTextField('HostNameOrIp', widget.onHostNameOrIpChanged),
        if (mountTypeLower == 'remote')
          ..._createTextField('ApiPort', widget.onApiPortChanged),
        if (mountTypeLower == 'remote')
          ..._createTextField(
            'EncryptionToken',
            widget.onEncryptionTokenChanged,
          ),
      ],
    );
  }
}
