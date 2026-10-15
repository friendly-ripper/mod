//! Build graph. So far only SDK provisioning: zig/ctx.zig holds the per-target compile rules and
//! zig/toolchains.zig the pinned SDKs.
const std = @import("std");
const builtin = @import("builtin");
const toolchains = @import("zig/toolchains.zig");

comptime {
    if (builtin.zig_version.order(.{ .major = 0, .minor = 16, .patch = 0 }) != .eq)
        @compileError("This build is pinned to Zig 0.16.0");
}

pub fn build(b: *std.Build) void {
    toolchains.addFetchSteps(b);
}
