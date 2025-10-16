// aurora_sweep.dart

import 'dart:math' as math;
import 'package:flutter/material.dart';

class AuroraSweep extends StatefulWidget {
  const AuroraSweep({
    super.key,
    this.enabled = true,
    this.duration = const Duration(seconds: 28),
    this.primaryAlphaA = 0.08, // default dimmer for crispness
    this.primaryAlphaB = 0.07,
    this.staticPhase = 0.25,
    this.radiusX = 0.85,
    this.radiusY = 0.35,
    this.beginYOffset = -0.55,
    this.endYOffset = 0.90,
  });

  final bool enabled;
  final Duration duration;
  final double primaryAlphaA;
  final double primaryAlphaB;
  final double staticPhase;
  final double radiusX;
  final double radiusY;
  final double beginYOffset;
  final double endYOffset;

  @override
  State<AuroraSweep> createState() => _AuroraSweepState();
}

class _AuroraSweepState extends State<AuroraSweep>
    with SingleTickerProviderStateMixin {
  late final AnimationController _c = AnimationController(
    vsync: this,
    duration: widget.duration,
  );

  @override
  void initState() {
    super.initState();
    if (widget.enabled) {
      _c.repeat();
    } else {
      _c.value = widget.staticPhase.clamp(0.0, 1.0);
    }
  }

  @override
  void didUpdateWidget(covariant AuroraSweep oldWidget) {
    super.didUpdateWidget(oldWidget);

    if (oldWidget.duration != widget.duration) {
      _c.duration = widget.duration;
    }

    if (widget.enabled && !_c.isAnimating) {
      _c.repeat();
    } else if (!widget.enabled && _c.isAnimating) {
      _c.stop();
      _c.value = widget.staticPhase.clamp(0.0, 1.0);
    }
  }

  @override
  void dispose() {
    _c.dispose();
    super.dispose();
  }

  (Alignment, Alignment) _alignmentsFromPhase(double t) {
    final theta = 2 * math.pi * t;
    final begin = Alignment(
      widget.radiusX * math.cos(theta),
      widget.beginYOffset + widget.radiusY * math.sin(theta),
    );
    final end = Alignment(
      widget.radiusX * math.cos(theta + math.pi / 2),
      widget.endYOffset + widget.radiusY * math.sin(theta + math.pi / 2),
    );
    return (begin, end);
  }

  @override
  Widget build(BuildContext context) {
    final scheme = Theme.of(context).colorScheme;

    Widget paint(double t) {
      final (begin, end) = _alignmentsFromPhase(t);
      return IgnorePointer(
        child: Container(
          decoration: BoxDecoration(
            gradient: LinearGradient(
              begin: begin,
              end: end,
              colors: [
                scheme.primary.withValues(alpha: widget.primaryAlphaA),
                Colors.transparent,
                scheme.primary.withValues(alpha: widget.primaryAlphaB),
              ],
              stops: const [0.0, 0.5, 1.0],
            ),
          ),
        ),
      );
    }

    if (!widget.enabled) {
      return paint(_c.value);
    }

    return AnimatedBuilder(
      animation: _c,
      builder: (_, _) {
        final t = _c.value;
        return paint(t);
      },
    );
  }
}
