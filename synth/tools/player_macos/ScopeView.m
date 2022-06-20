#import "ScopeView.h"

@interface ScopeView () {
  double phase_;
}
@end

@implementation ScopeView

#if TARGET_OS_OSX
- (ScopeView *)initWithFrame:(NSRect)frameRect {
#elif TARGET_OS_IOS
- (ScopeView *)initWithFrame:(CGRect)frameRect {
#endif
  self = [super initWithFrame:frameRect];
  if (self) {
    self->phase_ = 0.0;
  }
  return self;
}

- (void)awakeFromNib {
  [super awakeFromNib];

  typedef void (^TimerBlock) (NSTimer *timer);
  TimerBlock block = ^(NSTimer *_Nonnull _) {
#if TARGET_OS_OSX
    self.needsDisplay = YES;
#elif TARGET_OS_IOS
    [self setNeedsDisplay];
#endif
    self->phase_ += 0.1;
  };
  NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)(1.0 / 60.0)
                                                   repeats:YES
                                                     block:block];
  // Ensure that the timer continues to fire if the user drags something.
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

#if TARGET_OS_OSX
- (void)drawRect:(NSRect)rect {
  CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
  CGColorRef color = [[NSColor labelColor] CGColor];
#elif TARGET_OS_IOS
- (void)drawRect:(CGRect)rect {
  CGContextRef context = UIGraphicsGetCurrentContext ();
  CGColorRef color = [[UIColor labelColor] CGColor];
#endif
  CGRect const bounds = [self bounds];
  CGFloat const boundsWidth = CGRectGetWidth (bounds);

  static CGFloat const lineWidth = 2.0;
  CGFloat const yScale = (CGRectGetHeight (bounds) - lineWidth) / 2.0;
  CGFloat const frequency = 0.05;

  typedef void (^DrawAxisBlock) (CGFloat pos, CGColorRef color);
  DrawAxisBlock drawAxis = ^(CGFloat pos, CGColorRef color) {
    CGContextSetStrokeColorWithColor (context, color);
    CGRect const r = {.origin = CGPointMake (0.0, pos), .size = CGSizeMake (boundsWidth, 0.0)};
    CGContextStrokeRect (context, r);
  };

  CGContextTranslateCTM (context, 0.0, yScale + (lineWidth / 2));
  CGContextScaleCTM (context, 1.0, yScale);

  CGContextSetLineWidth (context, 1.0 / yScale);
  drawAxis (1.0, [[NSColor systemBlueColor] CGColor]);
  drawAxis (0.0, [[NSColor systemGreenColor] CGColor]);
  drawAxis (-1.0, [[NSColor systemRedColor] CGColor]);

  CGContextBeginPath (context);
  CGContextSetStrokeColorWithColor (context, color);
  CGContextSetLineWidth (context, lineWidth / yScale);
  CGContextMoveToPoint (context, 0.0, (CGFloat)(sin (phase_)));
  for (unsigned t = 1U, stepCount = (unsigned)(boundsWidth + 0.5); t <= stepCount; ++t) {
    CGContextAddLineToPoint (context, t, (CGFloat)(sin (t * frequency + phase_)));
  }
  CGContextStrokePath (context);
}

@end
