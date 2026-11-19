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
* `-s '<processor>...'`: specifies strategy

### `<smt>`:
* `(` (`z3`|`cvc5`)  `<smt-logic>` [`:tee <file>`] [`:let <bool>`] `)`

### `<smt-logic>`:
* e.g. `QF_LIA`, see [https://smt-lib.org/logics.shtml]().

### `<processor>`
* `(rule-remover` `<mono-order>` [`:smt <smt>`] `)`
* `(dp-remover` `<order>` [`:smt <smt>`] [`:usable` (`true`|`false`)] [`:edge` (`true`|`false`)] `)`

### `<order>`:
* `(` (`mono-sum`|`sum`|`max`|`maxsum`|`imaxsum`|`mat2b`|`sum_negsum`|`imaxsum_negsum`|`template...`) [`:log` (`debug`| ...)] `)`
* `(path-order `[`:weight <order>`] [`:status` (`straight`|`(map [<num>])`)] `)`


