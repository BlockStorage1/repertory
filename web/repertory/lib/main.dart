import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/widgets/mount_list_widget.dart';
import 'package:repertory/widgets/mount_settings.dart';

void main() {
  runApp(const MyApp());
}

class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: constants.appTitle,
      theme: ThemeData(
        brightness: Brightness.light,
        colorScheme: ColorScheme.fromSeed(
          seedColor: Colors.deepOrange,
          brightness: Brightness.light,
        ),
      ),
      themeMode: ThemeMode.dark,
      darkTheme: ThemeData(
        useMaterial3: true,
        brightness: Brightness.dark,
        colorScheme: ColorScheme.fromSeed(
          brightness: Brightness.dark,
          onSurface: Colors.white70,
          seedColor: Colors.deepOrange,
          surface: Color.fromARGB(255, 32, 33, 36),
          surfaceContainerLow: Color.fromARGB(255, 41, 42, 45),
        ),
      ),
      initialRoute: '/',
      routes: {'/': (context) => const MyHomePage(title: constants.appTitle)},
      onGenerateRoute: (settings) {
        if (settings.name != '/settings') {
          return null;
        }

        final mount = settings.arguments as Mount;
        return MaterialPageRoute(
          builder: (context) {
            return Scaffold(
              appBar: AppBar(
                backgroundColor: Theme.of(context).colorScheme.inversePrimary,
                title: Text(
                  '${initialCaps(mount.type)} [${formatMountName(mount.type, mount.name)}] Settings',
                ),
              ),
              body: MountSettingsWidget(mount: mount),
            );
          },
        );
      },
    );
  }
}

class MyHomePage extends StatefulWidget {
  final String title;
  const MyHomePage({super.key, required this.title});

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        leading: const Icon(Icons.storage),
        title: Text(widget.title),
      ),
      body: ChangeNotifierProvider(
        create: (context) => MountList(),
        child: MountListWidget(),
      ),
      floatingActionButton: FloatingActionButton(
        onPressed: () {},
        tooltip: 'Add',
        child: const Icon(Icons.add),
      ),
    );
  }
}
