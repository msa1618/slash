# Slash is in Beta!
All the basics of a shell were added, and finished on 17 June 2025. Slash is not in Alpha anymore. This means:
1. Slash is almost eligible to become a legitimate shell
2. It's no longer plagued with errors
3. You can (almost) use Slash like any other shell

## The Slash Philosophy
Every Slash utility shas to follow a unified philosophy:
1. Errors and notices should be color coded
2. The utility must be vibrant, but not garnish. It can follow a theme, or a color set. ANSI color codes should not be written raw and should be decalred in a header file for all to be used. If possible, let it comply with the current Slash theme set. (coming soon)
3. The utility must fulfill its purpose very well. It should be simple but feature rich at the same time.
4. The code should be **clean, simple to read**, and should not make the reader think hard
5. If it is a complex utility, the functions must be split into separate files. For example, an editor designed for programming must be split into editor.h, autocomplete.h, file_operations.h, vcs.h, ...
6. Do not let the library you are using limit your feature set. If that is the case, it is time to either make your own, or choose another library
7. Avoid storing data in binary files, thinking it's cool and edgy. Store settings in JSON, YAML, TOML, or any easy to understand types. 

# Want to support me?
You can donate to me at [my BuyMeACoffee Page](https://buymeacoffee.com/msa_1618). Even if it's just a dollar, it would really encourage me to continue and I would greatly appreciate it.
