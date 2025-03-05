import 'package:flutter/material.dart';

class AddMountWidget extends StatefulWidget {
  final bool allowEncrypt;
  final String mountType;
  final void Function(String? newName) onNameChanged;
  final void Function(String? newType) onTypeChanged;

  final _items = <String>["S3", "Sia"];

  AddMountWidget({
    super.key,
    required this.allowEncrypt,
    required this.mountType,
    required this.onNameChanged,
    required this.onTypeChanged,
  }) {
    if (allowEncrypt) {
      _items.insert(0, "Encrypt");
    }
  }

  @override
  State<AddMountWidget> createState() => _AddMountWidgetState();
}

class _AddMountWidgetState extends State<AddMountWidget> {
  String? _mountType;

  @override
  void initState() {
    _mountType = widget.mountType;
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
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
                  widget._items.map<DropdownMenuItem<String>>((item) {
                    return DropdownMenuItem<String>(
                      value: item,
                      child: Text(item),
                    );
                  }).toList(),
            ),
          ],
        ),
        const SizedBox(height: 10),
        if (_mountType != "Encrypt")
          Text(
            'Configuration Name',
            textAlign: TextAlign.left,
            style: TextStyle(
              color: Theme.of(context).colorScheme.onSurface,
              fontWeight: FontWeight.bold,
            ),
          ),
        if (_mountType != "Encrypt")
          TextField(
            autofocus: true,
            decoration: InputDecoration(),
            onChanged: widget.onNameChanged,
          ),
      ],
    );
  }
}
