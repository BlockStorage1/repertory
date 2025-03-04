import 'package:flutter/material.dart';
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/mount.dart';
import 'package:repertory/models/mount_list.dart';
import 'package:repertory/widgets/add_mount_widget.dart';
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
  bool _allowAdd = true;
  String _mountType = "S3";
  String _mountName = "";

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
        onPressed:
            _allowAdd
                ? () {
                  showDialog(
                    context: context,
                    builder: (BuildContext context) {
                      return AlertDialog(
                        title: const Text('Add Mount'),
                        content: Consumer<MountList>(
                          builder: (_, mountList, __) {
                            return AddMountWidget(
                              allowEncrypt:
                                  !mountList.items.contains(
                                    (item) => item.type == "encrypt",
                                  ),
                              mountName: _mountName,
                              mountType: _mountType,
                              onNameChanged:
                                  (mountName) => setState(
                                    () => _mountName = mountName ?? "",
                                  ),
                              onTypeChanged:
                                  (mountType) => setState(
                                    () => _mountType = mountType ?? "S3",
                                  ),
                            );
                          },
                        ),
                        actions: [
                          TextButton(
                            child: const Text('Cancel'),
                            onPressed: () {
                              setState(() {
                                _mountType = "S3";
                                _mountName = "";
                              });
                              Navigator.of(context).pop();
                            },
                          ),
                          TextButton(
                            child: const Text('OK'),
                            onPressed: () {
                              setState(() => _allowAdd = false);

                              Provider.of<MountList>(context, listen: false)
                                  .add(_mountType, _mountName)
                                  .then((_) {
                                    setState(() {
                                      _allowAdd = true;
                                      _mountType = "S3";
                                      _mountName = "";
                                    });
                                  })
                                  .catchError((_) {
                                    setState(() {
                                      _allowAdd = true;
                                    });
                                  });

                              Navigator.of(context).pop();
                            },
                          ),
                        ],
                      );
                    },
                  );
                }
                : null,
        tooltip: 'Add Mount',
        child: const Icon(Icons.add),
      ),
    );
  }
}
