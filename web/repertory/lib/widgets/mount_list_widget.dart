import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/widgets/mount_widget.dart';

class MountListWidget extends StatelessWidget {
  const MountListWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<MountList>(
      builder: (context, MountList mountList, _) {
        return ListView.builder(
          itemBuilder: (context, idx) {
            return ChangeNotifierProvider(
              create: (context) => mountList.items[idx],
              key: ValueKey(mountList.items[idx].id),
              child: Padding(
                padding: EdgeInsets.only(
                  bottom:
                      idx == mountList.items.length - 1
                          ? 0.0
                          : constants.padding,
                ),
                child: const MountWidget(),
              ),
            );
          },
          itemCount: mountList.items.length,
        );
      },
    );
  }
}
