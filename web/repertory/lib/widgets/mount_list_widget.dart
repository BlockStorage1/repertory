import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/widgets/mount_widget.dart';

class MountListWidget extends StatelessWidget {
  const MountListWidget({super.key});

  @override
  Widget build(BuildContext context) {
    return Consumer<MountList>(
      builder: (context, mountList, _) {
        return ListView.builder(
          itemBuilder: (context, idx) {
            return ChangeNotifierProvider(
              create: (context) => Mount(mountList.items[idx]),
              child: const MountWidget(),
            );
          },
          itemCount: mountList.items.length,
        );
      },
    );
  }
}
