import 'package:flutter/material.dart';

class AddMountWidget extends StatefulWidget {
  final String mountType;
  final void Function(String name, String? value) onDataChanged;

  const AddMountWidget({
    super.key,
    required this.mountType,
    required this.onDataChanged,
  });

  @override
  State<AddMountWidget> createState() => _AddMountWidgetState();
}

class _AddMountWidgetState extends State<AddMountWidget> {
  static const _items = <String>['Encrypt', 'Remote', 'S3', 'Sia'];
  static const _padding = 15.0;

  String? _mountType;

  @override
  void initState() {
    _mountType = widget.mountType;
    super.initState();
  }

  List<Widget> _createTextField(
    String title,
    String dataName, {
    String? value,
  }) {
    if (value != null) {
      widget.onDataChanged(dataName, value);
    }

    return [
      const SizedBox(height: _padding),
      Text(
        title,
        textAlign: TextAlign.left,
        style: TextStyle(
          color: Theme.of(context).colorScheme.onSurface,
          fontWeight: FontWeight.bold,
        ),
      ),
      TextField(
        autofocus: dataName == 'Name',
        decoration: InputDecoration(),
        onChanged: (value) => widget.onDataChanged(dataName, value),
        controller: TextEditingController(text: value),
      ),
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
                widget.onDataChanged('Provider', value);
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
        if (mountTypeLower != 'remote')
          ..._createTextField('Configuration Name', 'Name'),
        if (mountTypeLower == 'encrypt') ..._createTextField('Path', 'Path'),
        if (mountTypeLower == 'sia')
          ..._createTextField('ApiPassword', 'ApiPassword'),
        if (mountTypeLower == 's3' || mountTypeLower == 'sia')
          ..._createTextField(
            'Bucket',
            'Bucket',
            value: mountTypeLower == 'sia' ? 'default' : null,
          ),
        if (mountTypeLower == 'remote' || mountTypeLower == 'sia')
          ..._createTextField(
            'HostNameOrIp',
            'HostNameOrIp',
            value: 'localhost',
          ),
        if (mountTypeLower == 'remote' || mountTypeLower == 'sia')
          ..._createTextField(
            'ApiPort',
            'ApiPort',
            value: mountTypeLower == 'sia' ? '9980' : null,
          ),
        if (mountTypeLower == 'remote')
          ..._createTextField('EncryptionToken', 'EncryptionToken'),
      ],
    );
  }
}
