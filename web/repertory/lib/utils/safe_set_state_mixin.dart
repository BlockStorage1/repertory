// safe_set_state_mixin.dart

import 'package:flutter/widgets.dart';

mixin SafeSetState<T extends StatefulWidget> on State<T> {
  @override
  void setState(VoidCallback fn) {
    if (!mounted) {
      return;
    }

    super.setState(fn);
  }
}
