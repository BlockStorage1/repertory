import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/models/auth.dart';
import 'package:repertory/widgets/mount_list_widget.dart';

class HomeScreen extends StatefulWidget {
  final String title;
  const HomeScreen({super.key, required this.title});

  @override
  State<HomeScreen> createState() => _HomeScreeState();
}

class _HomeScreeState extends State<HomeScreen> {
  @override
  Widget build(context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        leading: IconButton(
          onPressed: () => Navigator.pushNamed(context, '/settings'),
          icon: const Icon(Icons.storage),
        ),
        title: Text(widget.title),
        actions: [
          Consumer<Auth>(
            builder: (context, auth, _) {
              return IconButton(
                icon: const Icon(Icons.logout),
                onPressed: () => auth.logoff(),
              );
            },
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(constants.padding),
        child: MountListWidget(),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () => Navigator.pushNamed(context, '/add'),
        tooltip: 'Add Mount',
        child: const Icon(Icons.add),
      ),
    );
  }

  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
