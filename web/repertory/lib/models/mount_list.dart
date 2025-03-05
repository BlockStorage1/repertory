import 'dart:convert';

import 'package:collection/collection.dart';
import 'package:flutter/foundation.dart';
import 'package:http/http.dart' as http;
import 'package:repertory/helpers.dart';
import 'package:repertory/types/mount_config.dart';

class MountList with ChangeNotifier {
  MountList() {
    _fetch();
  }

  List<MountConfig> _mountList = [];

  UnmodifiableListView<MountConfig> get items =>
      UnmodifiableListView<MountConfig>(_mountList);

  Future<void> _fetch() async {
    final response = await http.get(
      Uri.parse('${getBaseUri()}/api/v1/mount_list'),
    );

    if (response.statusCode == 200) {
      List<MountConfig> nextList = [];

      jsonDecode(response.body).forEach((key, value) {
        nextList.addAll(
          value.map((name) => MountConfig.fromJson(key, name)).toList(),
        );
      });
      _sort(nextList);
      _mountList = nextList;

      notifyListeners();
      return;
    }
  }

  void _sort(list) {
    list.sort((a, b) {
      final res = a.type.compareTo(b.type);
      if (res != 0) {
        return res;
      }

      return a.name.compareTo(b.name);
    });
  }

  Future<void> add(
    String type,
    String name, {
    String? apiPassword,
    String? apiPort,
    String? bucket,
    String? encryptionToken,
    String? hostNameOrIp,
    String? path,
  }) async {
    await http.post(
      Uri.parse(
        Uri.encodeFull(
          '${getBaseUri()}/api/v1/add_mount?name=$name&type=$type&bucket=$bucket'
          '&path=$path&apiPassword=$apiPassword&apiPort=$apiPort&hostNameOrIp=$hostNameOrIp'
          '&encryptionToken=$encryptionToken',
        ),
      ),
    );

    return _fetch();
  }

  void remove(String name) {
    _mountList.removeWhere((item) => item.name == name);

    notifyListeners();
  }
}
