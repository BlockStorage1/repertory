import 'package:flutter/material.dart';
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
        leading: const Icon(Icons.storage),
        title: Text(widget.title),
      ),
      body: MountListWidget(),
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
