import 'package:flutter/material.dart';

class AddMountWidget extends StatefulWidget {
  final bool allowEncrypt;
  const AddMountWidget({super.key, required this.allowEncrypt});

  @override
  State<AddMountWidget> createState() => _AddMountWidgetState();
}

class _AddMountWidgetState extends State<AddMountWidget> {
  @override
  Widget build(BuildContext context) {
    var items = ["S3", "Sia"];
    if (widget.allowEncrypt) {
      items.insert(0, "Encrypt");
    }

    return Column(
      mainAxisSize: MainAxisSize.min,
      children: [
        DropdownButton<String>(
          value: "S3",
          onChanged: (newValue) {},
          items:
              items.map<DropdownMenuItem<String>>((item) {
                return DropdownMenuItem<String>(value: item, child: Text(item));
              }).toList(),
        ),
        TextField(decoration: InputDecoration(labelText: 'Name')),
      ],
    );
  }
}
