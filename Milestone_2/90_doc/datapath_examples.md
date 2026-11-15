# Datapath Examples by Instruction Type

Ví dụ số cụ thể cho từng nhóm lệnh của `singlecycle`. File này bổ sung cho
[datapath_notes_by_instruction_type.md](datapath_notes_by_instruction_type.md)
(lý thuyết và tín hiệu control); ở đây chỉ tập trung vào **mã máy và giá trị
chạy qua datapath**.

Quy ước:

- `PC` bắt đầu từ `0x8000_0000` nếu không ghi khác.
- Mã máy được tính tay từ định dạng RV32I; nên kiểm tra lại bằng assembler
  nếu cần dùng cho test.
- `imm` là giá trị sau `immgen` (đã sign-extend).

## 0. Cách `immgen` ghép immediate

Bit dấu luôn là `instr[31]`.

| `imm_sel` | Kiểu | Ghép từ | Bit 0 |
|---|---|---|---|
| 0 | I | `[31:20]` | lấy từ lệnh |
| 1 | S | `[31:25]`, `[11:7]` | lấy từ lệnh |
| 2 | B | `[31]`, `[7]`, `[30:25]`, `[11:8]` | luôn `0` |
| 3 | U | `[31:12]` rồi 12 bit `0` | luôn `0` |
| 4 | J | `[31]`, `[19:12]`, `[20]`, `[30:21]` | luôn `0` |

## 1. R-type

### `add x3, x1, x2`

- Mã máy: `0x002081B3`
  (`funct7=0000000 rs2=2 rs1=1 funct3=000 rd=3 opcode=0110011`).
- Giả sử `x1 = 5`, `x2 = 7`.

```text
rs1_data = 5 ─▶ op_a ─┐
                      ├─▶ ALU(ADD) ─▶ alu_data = 12 ─▶ wb_data ─▶ x3
rs2_data = 7 ─▶ op_b ─┘
```

| Tín hiệu | Giá trị |
|---|---|
| `opa_sel`, `opb_sel` | 0, 0 |
| `alu_op` | ADD |
| `wb_sel` | 0 |
| `rd_wren` | 1 |
| `pc_sel` | 0, nên `PC = 0x8000_0004` |

### `sub x3, x1, x2`

- Mã máy: `0x402081B3`. Chỉ khác `instr[30] = 1` (`funct7 = 0100000`).
- Với `x1 = 5`, `x2 = 7`: `alu_data = 0xFFFF_FFFE` (tức -2), ghi vào `x3`.

## 2. I-type ALU

### `addi x5, x1, -1`

- Mã máy: `0xFFF08293`; `imm = 0xFFF` được sign-extend thành `0xFFFF_FFFF`.
- Với `x1 = 10`: `10 + (-1) = 9`, ghi vào `x5`.

```text
rs1_data = 10 ─────▶ op_a ─┐
                           ├─▶ ALU(ADD) ─▶ 9 ─▶ x5
imm = 0xFFFF_FFFF ─▶ op_b ─┘
```

Tín hiệu: `imm_sel = 0`, `opa_sel = 0`, `opb_sel = 1`, `wb_sel = 0`,
`rd_wren = 1`.

### `srai x5, x1, 2`

- Mã máy: `0x4020D293`; `instr[30] = 1` chọn shift số học.
- Với `x1 = 0xFFFF_FFF0` (-16): kết quả `0xFFFF_FFFC` (-4).
- Shift amount là `op_b[4:0] = 2`.

## 3. Load

### `lw x5, 8(x1)`

- Mã máy: `0x0080A283`.
- Giả sử `x1 = 0x1000_0000`, và dmem tại địa chỉ đó chứa `0xDEAD_BEEF`.

```text
x1 = 0x1000_0000 ─▶ op_a ─┐
                          ├─▶ ALU ─▶ 0x1000_0008 ─▶ o_dmem_addr ─▶ DMEM
imm = 8 ─────────▶ op_b ─┘
i_dmem_rdata = 0xDEAD_BEEF ─▶ LSU (funct3=010: lấy cả word) ─▶ ld_data ─▶ x5
```

Tín hiệu: `imm_sel = 0`, `wb_sel = 1`, `rd_wren = 1`,
`mem_wren = 0` (nên `o_dmem_bmsk = 0000`).

### `lb x5, 1(x1)` và `lbu x5, 1(x1)`

- Mã máy `lb`: `0x00108283`. `lbu` chỉ đổi `funct3` thành `100`.
- `x1 = 0x1000_0000` nên địa chỉ là `0x1000_0001`, `addr[1:0] = 01`.
- Giả sử DMEM trả `0x1234_F0AB`. LSU chọn byte `[15:8] = 0xF0`.

| Lệnh | Mở rộng | Kết quả |
|---|---|---|
| `lb` | sign-extend | `0xFFFF_FFF0` |
| `lbu` | zero-extend | `0x0000_00F0` |

## 4. Store

### `sw x2, 8(x1)`

- Mã máy: `0x0020A423`.
- `x1 = 0x1000_0000`, `x2 = 0xCAFE_BABE`.

```text
x1 + imm(S)=8 ─▶ ALU ─▶ 0x1000_0008 ─▶ LSU ─▶ o_dmem_addr
x2 ───────────────────────────────────▶ LSU ─▶ o_dmem_wdata = 0xCAFE_BABE
                                              o_dmem_bmsk  = 1111
                                              o_dmem_wren  = 1
```

Tín hiệu: `imm_sel = 1`, `rd_wren = 0`, `mem_wren = 1`.

### `sb x2, 3(x1)`

- Mã máy: `0x002081A3`.
- `x1 = 0x1000_0000`, `x2 = 0x0000_00AB`. Địa chỉ `0x1000_0003` có
  `addr[1:0] = 11`.
- LSU lặp byte thấp 4 lần: `o_dmem_wdata = 0xABAB_ABAB`, và
  `o_dmem_bmsk = 1000`. Chỉ byte 3 được ghi.

### `sh x2, 2(x1)`

- Mã máy: `0x00209123`.
- `addr[1] = 1` nên `o_dmem_bmsk = 1100`. `o_dmem_wdata = {2{x2[15:0]}}`.

## 5. Branch

### `beq x1, x2, 16` (nhảy tiến)

- Mã máy: `0x00208863`; `imm = 16`.
- `PC = 0x8000_0000`.

```text
rs1_data, rs2_data ─▶ BRC ─▶ br_eq ─▶ controller ─▶ pc_sel
PC ─▶ op_a ─┐
imm=16 ─▶ op_b ─┴▶ ALU(ADD) ─▶ 0x8000_0010 (branch target)
```

| Điều kiện | `pc_sel` | PC mới |
|---|---|---|
| `x1 == x2` | 1 | `0x8000_0010` |
| `x1 != x2` | 0 | `0x8000_0004` |

### `bne x1, x2, -8` (nhảy lùi)

- Mã máy: `0xFE209CE3`; `imm = 0xFFFF_FFF8`.
- `PC = 0x8000_0020`. Nếu `x1 != x2`, PC mới là `0x8000_0018`.

### `blt` và `bltu`

Với `x1 = 0xFFFF_FFFF` và `x2 = 1`:

| Lệnh | `funct3` | `br_unsigned` | So sánh | Nhảy? |
|---|---|---|---|---|
| `blt` | `100` | 0 | -1 < 1 | Có |
| `bltu` | `110` | 1 | 4294967295 < 1 | Không |

## 6. JAL

### `jal x1, 16`

- Mã máy: `0x010000EF`.
- `PC = 0x8000_0000`.

```text
PC ─▶ op_a ─┐
imm=16 ─▶ op_b ─┴▶ ALU(ADD) ─▶ 0x8000_0010 ─▶ pc_next   (pc_sel = 1)
adder: PC + 4 = 0x8000_0004 ─▶ wb_data ─▶ x1             (wb_sel = 2)
```

Kết quả: `x1 = 0x8000_0004`, `PC = 0x8000_0010`.

### `jal x1, -8`

- Mã máy: `0xFF9FF0EF`; `imm = 0xFFFF_FFF8`.
- `PC = 0x8000_0020` cho ra `x1 = 0x8000_0024`, `PC = 0x8000_0018`.

### `jal x0, label` (lệnh `j`)

`rd = x0` nên return address bị bỏ, nhưng PC vẫn nhảy.

## 7. JALR

### `jalr x1, 0(x5)`

- Mã máy: `0x000280E7`.
- `PC = 0x8000_0030`, `x5 = 0x8000_0101`.

```text
x5 = 0x8000_0101 ─▶ op_a ─┐
imm = 0 ─────────▶ op_b ─┴▶ ALU(ADD) ─▶ 0x8000_0101
                                     └▶ {alu_data[31:1], 1'b0} = 0x8000_0100 ─▶ PC
adder: PC + 4 = 0x8000_0034 ─▶ x1
```

Kết quả: `PC = 0x8000_0100` (bit 0 bị xóa), `x1 = 0x8000_0034`.

### `ret`

`ret` là `jalr x0, 0(x1)`, mã máy `0x00008067`. PC lấy từ `x1`, và `x0`
không nhận return address.

## 8. LUI và AUIPC

### `lui x5, 0x12345`

- Mã máy: `0x123452B7`.

```text
imm(U) = 0x12345000 ─▶ wb_data (wb_sel = 3) ─▶ x5
```

Kết quả `x5 = 0x1234_5000`. Không dùng ALU.

### `auipc x5, 0x1`

- Mã máy: `0x00001297`.
- `PC = 0x8000_0008`.

```text
PC = 0x8000_0008 ─▶ op_a ─┐
imm = 0x0000_1000 ─▶ op_b ─┴▶ ALU(ADD) ─▶ 0x8000_1008 ─▶ x5   (wb_sel = 0)
```

Kết quả `x5 = 0x8000_1008`, và `PC` vẫn tăng thêm 4.

## 9. Lệnh không hợp lệ

Ví dụ `funct7 = 0100000` với `and` (đáng lẽ chỉ `0000000`):

- `valid = 0` nên `o_db_valid = 0`.
- `rd_wren`, `mem_wren` và `pc_sel` đều bị mask về 0.
- Không có gì được ghi; `PC` đi tới `PC + 4`.

## 10. Bảng tóm tắt nhanh

| Lệnh ví dụ | Mã máy | ALU tính | Kết quả |
|---|---|---|---|
| `add x3,x1,x2` | `0x002081B3` | `rs1 + rs2` | `x3 = x1 + x2` |
| `sub x3,x1,x2` | `0x402081B3` | `rs1 - rs2` | `x3 = x1 - x2` |
| `addi x5,x1,-1` | `0xFFF08293` | `rs1 + imm` | `x5 = x1 - 1` |
| `srai x5,x1,2` | `0x4020D293` | `rs1 >>> 2` | `x5 = x1 >>> 2` |
| `lw x5,8(x1)` | `0x0080A283` | `rs1 + 8` (địa chỉ) | `x5 = mem[x1+8]` |
| `lb x5,1(x1)` | `0x00108283` | `rs1 + 1` (địa chỉ) | byte sign-extend |
| `sw x2,8(x1)` | `0x0020A423` | `rs1 + 8` (địa chỉ) | `mem[x1+8] = x2` |
| `sb x2,3(x1)` | `0x002081A3` | `rs1 + 3` (địa chỉ) | ghi 1 byte |
| `sh x2,2(x1)` | `0x00209123` | `rs1 + 2` (địa chỉ) | ghi 2 byte |
| `beq x1,x2,16` | `0x00208863` | `PC + 16` | nhảy nếu bằng |
| `bne x1,x2,-8` | `0xFE209CE3` | `PC - 8` | nhảy nếu khác |
| `jal x1,16` | `0x010000EF` | `PC + 16` | `x1 = PC+4` |
| `jal x1,-8` | `0xFF9FF0EF` | `PC - 8` | `x1 = PC+4` |
| `jalr x1,0(x5)` | `0x000280E7` | `rs1 + 0` | `x1 = PC+4` |
| `lui x5,0x12345` | `0x123452B7` | không dùng | `x5 = 0x12345000` |
| `auipc x5,0x1` | `0x00001297` | `PC + 0x1000` | `x5 = PC + 0x1000` |
