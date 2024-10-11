# Migrate to new agent api

This is a program that translates sc-machine usage from old API into new API.

Changed files include:

- cmake instructions that mention codegen;
- keynodes files with codegen;
- module files with codegen;
- agent files with codegen;
- in-line declarations of `SC_PROPERTY`;
- `#include` statements with removed files;
- outdated `scAgentsCommon::CoreKeynodes` usages;
- outdated sc-events usages;
- deprecated methods usages;
- deprecated sc-types usages;
- `utils::AgentUtils` usages;
- outdated agent registration and unregistration statements.

Example of usage:
```shell
./scripts/migration/migrate_to_new_agent_api.sh path/to/folder/with/project
```

There may be a situation when this program won't be able to replace code correctly, these places in code are marked with `//todo(codegen-removal):`

### **_If you want to run this program you will need Java with version 8 or higher installed_**