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

  typedef void (^timerBlock) (NSTimer *timer);
  timerBlock block = ^(NSTimer *_Nonnull _) {
#if TARGET_OS_OSX
    self.needsDisplay = YES;
#elif TARGET_OS_IOS
    [self setNeedsDisplay];
#endif
    self->phase_ += 0.1;
  };
  NSTimer *timer = [NSTimer scheduledTimerWithTimeInterval:(NSTimeInterval)1.0 / 60.0
                                                   repeats:YES
                                                     block:block];
  // Ensure that the timer continues to fire if the user drags something.
  [[NSRunLoop currentRunLoop] addTimer:timer forMode:NSRunLoopCommonModes];
}

- (void)drawRect:(CGRect)rect {
  CGRect const bounds = [self bounds];
#if TARGET_OS_OSX
  CGContextRef context = [[NSGraphicsContext currentContext] CGContext];
  CGColorRef color = [[NSColor labelColor] CGColor];
#elif TARGET_OS_IOS
  CGContextRef context = UIGraphicsGetCurrentContext ();
  CGColorRef color = [[UIColor labelColor] CGColor];
#endif

  CGFloat const amplitude = CGRectGetHeight (bounds) / 2.0;
  CGFloat const frequency = 0.05;

  CGContextSetStrokeColorWithColor (context, color);
  CGContextSetLineWidth (context, 2.0);
  CGContextTranslateCTM (context, 0.0, amplitude);

  CGContextBeginPath (context);
  CGContextMoveToPoint (context, 0.0, (CGFloat)(amplitude * sin (phase_)));
  for (unsigned t = 1U, stepCount = (unsigned)CGRectGetWidth (bounds); t <= stepCount; ++t) {
    CGContextAddLineToPoint (context, t, (CGFloat)(amplitude * sin (t * frequency + phase_)));
  }
  CGContextStrokePath (context);
}

@end
