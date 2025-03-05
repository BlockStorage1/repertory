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

  String? _mountType;

  @override
  void initState() {
    _mountType = widget.mountType;
    super.initState();
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
            const SizedBox(width: 10),
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
        const SizedBox(height: 10),
        Text(
          'Configuration Name',
          textAlign: TextAlign.left,
          style: TextStyle(
            color: Theme.of(context).colorScheme.onSurface,
            fontWeight: FontWeight.bold,
          ),
        ),
        TextField(
          autofocus: true,
          decoration: InputDecoration(),
          onChanged: widget.onNameChanged,
        ),
        if (mountTypeLower == 'encrypt')
          Text(
            'Path',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 'encrypt')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onPathChanged,
          ),
        if (mountTypeLower == 'sia')
          Text(
            'ApiAuth',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 'sia')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onApiAuthChanged,
          ),
        if (mountTypeLower == 's3' || mountTypeLower == 'sia')
          Text(
            'Bucket',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 's3' || mountTypeLower == 'sia')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onBucketChanged,
          ),
        if (mountTypeLower == 'remote')
          Text(
            'HostNameOrIp',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 'remote')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onHostNameOrIpChanged,
          ),
        if (mountTypeLower == 'remote')
          Text(
            'ApiPort',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 'remote')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onApiPortChanged,
          ),
        if (mountTypeLower == 'remote')
          Text(
            'EncryptionToken',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (mountTypeLower == 'remote')
          TextField(
            decoration: InputDecoration(),
            onChanged: widget.onEncryptionTokenChanged,
          ),
      ],
    );
  }
}
