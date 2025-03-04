import 'package:flutter/material.dart';

class AddMountWidget extends StatelessWidget {
  final bool allowEncrypt;
  final String mountName;
  final String mountType;
  final void Function(String? newName) onNameChanged;
  final void Function(String? newType) onTypeChanged;
  const AddMountWidget({
    super.key,
    required this.allowEncrypt,
    required this.mountName,
    required this.mountType,
    required this.onNameChanged,
    required this.onTypeChanged,
  });

  @override
  Widget build(BuildContext context) {
    var items = ["S3", "Sia"];
    if (allowEncrypt) {
      items.insert(0, "Encrypt");
    }

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        DropdownButton<String>(
          value: mountType,
          onChanged: onTypeChanged,
          items:
              items.map<DropdownMenuItem<String>>((item) {
                return DropdownMenuItem<String>(value: item, child: Text(item));
              }).toList(),
        ),
        TextField(
          decoration: InputDecoration(labelText: 'Name'),
          onChanged: onNameChanged,
        ),
      ],
    );
  }
}
