import { Project } from 'ts-morph';

const project = new Project({
  tsConfigFilePath: './src/typeorm/tsconfig.json',
});

const sourceFiles = project.addSourceFilesAtPaths(
  './src/typeorm/entities/*.ts',
);
const outputDir = 'types';

sourceFiles.forEach((sourceFile) => {
  const filePath = `${outputDir}/${sourceFile.getBaseNameWithoutExtension()}.d.ts`;
  sourceFile.copy(filePath, { overwrite: true });
});

project.save().then(() => {
  console.log('Type definitions generated.');
});
