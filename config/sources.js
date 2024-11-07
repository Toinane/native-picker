const { readdirSync } = require('node:fs');
const { join } = require('node:path');

const walk = (dir) => {
  const entries = readdirSync(dir, { withFileTypes: true });
  return entries.map((entry) => {
    const path = join(dir, entry.name);
    return entry.isDirectory() ? walk(path) : path;
  });
}

console.log(
  walk("./src")
    .flat(Number.POSITIVE_INFINITY)
    .filter((file) => file.endsWith(".cc"))
    .map((file) => file.replace(/\\/g, "/"))
    .join(" ")
)