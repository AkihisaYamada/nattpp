# NaTT++

This is a termination tool, successor of [Nagoya Termination Tool](https://github.com/AkihisaYamada/natt). The tool tries to analyze termination of term rewrite systems (TRSs), written in [the ARI format](https://project-coco.uibk.ac.at/ARI/trs.php).


## Build

Install `make` and `c++`, and run `make`.

## Usage

```bash
natt++ [<option>...] [<input.ari>]
```
### `<option>`:
* `-smt '<smt>'`: specifies default SMT solver
* `-r/-d/-a '<order>'`: specifies rule removal / DP removal / both

### `<smt>`:
* `(` (`z3`|`cvc5`)  `<smt-logic>` [`:tee <file>`] [`:let <bool>`] `)`

### `<smt-logic>`:
* e.g. `QF-LIA`, see [https://smt-lib.org/logics.shtml]().

### `<order>`:
* `(` (`mono-sum`|`sum`|`max`|`template...`) [`:log` (`debug`| ...)] [`:smt <smt>`] `)`
* `(path-order `[`:weight <order>`] [`:status` (`straight`|`(map [<num>])`)] `)`


