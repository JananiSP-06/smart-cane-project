const { getDefaultConfig } = require("expo/metro-config");

const config = getDefaultConfig(__dirname);

// Fix for Firebase JS SDK: Metro's newer "package.json exports" resolution
// (enabled by default since Expo SDK 53) fails to correctly resolve
// Firebase's .cjs files, causing "Component auth has not been registered
// yet" at runtime. Disabling it falls back to the older, compatible
// resolution method.
config.resolver.unstable_enablePackageExports = false;
config.resolver.sourceExts = [...config.resolver.sourceExts, "cjs"];

module.exports = config;