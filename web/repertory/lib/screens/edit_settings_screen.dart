// edit_settings_screen.dart

import 'dart:convert' show jsonDecode, jsonEncode;

import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;
import 'package:provider/provider.dart';
import 'package:repertory/constants.dart' as constants;
import 'package:repertory/helpers.dart';
import 'package:repertory/models/auth.dart';
import 'package:repertory/utils/safe_set_state_mixin.dart';
import 'package:repertory/widgets/app_scaffold.dart';
import 'package:repertory/widgets/ui_settings.dart';

class EditSettingsScreen extends StatefulWidget {
  final String title;
  const EditSettingsScreen({super.key, required this.title});

  @override
  State<EditSettingsScreen> createState() => _EditSettingsScreenState();
}

class _EditSettingsScreenState extends State<EditSettingsScreen>
    with SafeSetState<EditSettingsScreen> {
  @override
  Widget build(BuildContext context) {
    return AppScaffold(
      title: widget.title,
      showBack: true,
      showUISettings: true,
      children: [
        Expanded(
          child: FutureBuilder<Map<String, dynamic>>(
            future: _grabSettings(),
            initialData: const <String, dynamic>{},
            builder: (context, snapshot) {
              if (!snapshot.hasData) {
                return const Center(child: CircularProgressIndicator());
              }

              return UISettingsWidget(
                origSettings: jsonDecode(jsonEncode(snapshot.requireData)),
                settings: snapshot.requireData,
                showAdvanced: false,
              );
            },
          ),
        ),
        const SizedBox(height: constants.padding),
      ],
    );
  }

  Future<Map<String, dynamic>> _grabSettings() async {
    try {
      final authProvider = Provider.of<Auth>(context, listen: false);
      final auth = await authProvider.createAuth();
      final response = await http.get(
        Uri.parse('${getBaseUri()}/api/v1/settings?auth=$auth'),
      );

      if (response.statusCode == 401) {
        authProvider.logoff();
        return {};
      }

      if (response.statusCode != 200) {
        return {};
      }

      return jsonDecode(response.body);
    } catch (e) {
      debugPrint('$e');
    }

    return {};
  }
}
