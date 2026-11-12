# Instruction Classification QA Cheat Sheet

Tài liệu tập trung vào cách phân loại instruction và luồng datapath tương ứng
trong `Milestone_2`.

## 1. Phân loại theo opcode

| Nhóm | Opcode | Format | Instruction được hỗ trợ |
|---|---|---|---|
| `LUI` | `0110111` | U | `LUI` |
| `AUIPC` | `0010111` | U | `AUIPC` |
| Jump | `1101111` | J | `JAL` |
| Jump register | `1100111` | I | `JALR` |
| Branch | `1100011` | B | `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU` |
| Load | `0000011` | I | `LB`, `LH`, `LW`, `LBU`, `LHU` |
| Store | `0100011` | S | `SB`, `SH`, `SW` |
| ALU immediate | `0010011` | I | `ADDI`, `SLTI`, `SLTIU`, `XORI`, `ORI`, `ANDI`, `SLLI`, `SRLI`, `SRAI` |
| ALU register | `0110011` | R | `ADD`, `SUB`, `SLL`, `SLT`, `SLTU`, `XOR`, `SRL`, `SRA`, `OR`, `AND` |

Instruction có `instruction[1:0] != 2'b11` hoặc encoding không được hỗ trợ
được đánh dấu invalid.

## 2. Các trường dùng để phân loại

```text
31       25 24     20 19     15 14    12 11      7 6       0
+----------+---------+---------+--------+----------+---------+
| funct7   |   rs2   |   rs1   | funct3 |    rd    | opcode  |
+----------+---------+---------+--------+----------+---------+
```

- `opcode = instruction[6:0]`: xác định nhóm instruction.
- `funct3 = instruction[14:12]`: xác định instruction con trong nhóm.
- `funct7 = instruction[31:25]`: phân biệt một số instruction R-type.
- `rs1 = instruction[19:15]`, `rs2 = instruction[24:20]`.
- `rd = instruction[11:7]`.
- `instruction[30] = funct7[5]`: phân biệt `ADD/SUB` và `SRL/SRA`.

## 3. Bảng `funct3` theo từng nhóm

### 3.1. R-type và ALU immediate

| `funct3` | R-type | I-type |
|---|---|---|
| `000` | `ADD` hoặc `SUB` | `ADDI` |
| `001` | `SLL` | `SLLI` |
| `010` | `SLT` | `SLTI` |
| `011` | `SLTU` | `SLTIU` |
| `100` | `XOR` | `XORI` |
| `101` | `SRL` hoặc `SRA` | `SRLI` hoặc `SRAI` |
| `110` | `OR` | `ORI` |
| `111` | `AND` | `ANDI` |

Với R-type:

```text
funct7 = 0000000: phép toán cơ bản
funct7 = 0100000: SUB hoặc SRA
```

Chỉ `funct3 = 000` và `funct3 = 101` được phép dùng cả hai giá trị trên.
Các `funct3` khác phải có `funct7 = 0000000`.

Với I-type:

- `SLLI` yêu cầu `funct7 = 0000000`.
- `SRLI` dùng `funct7 = 0000000`.
- `SRAI` dùng `funct7 = 0100000`.
- Các instruction I-type ALU còn lại không dùng `funct7` để chọn phép toán.

### 3.2. Branch

| `funct3` | Instruction | Điều kiện |
|---|---|---|
| `000` | `BEQ` | `rs1 == rs2` |
| `001` | `BNE` | `rs1 != rs2` |
| `100` | `BLT` | signed `rs1 < rs2` |
| `101` | `BGE` | signed `rs1 >= rs2` |
| `110` | `BLTU` | unsigned `rs1 < rs2` |
| `111` | `BGEU` | unsigned `rs1 >= rs2` |

`funct3 = 010` và `011` không được hỗ trợ trong controller này.

### 3.3. Load

| `funct3` | Instruction | Mở rộng dữ liệu |
|---|---|---|
| `000` | `LB` | sign-extend byte |
| `001` | `LH` | sign-extend halfword |
| `010` | `LW` | word 32 bit |
| `100` | `LBU` | zero-extend byte |
| `101` | `LHU` | zero-extend halfword |

`funct3 = 011`, `110`, `111` không hợp lệ cho nhóm load trong thiết kế này.

### 3.4. Store

| `funct3` | Instruction | Số byte ghi |
|---|---|---:|
| `000` | `SB` | 1 |
| `001` | `SH` | 2 |
| `010` | `SW` | 4 |

Các giá trị `funct3` khác không hợp lệ cho nhóm store.

## 4. Phân loại theo datapath

### Nhóm A — ALU register: R-type

Ví dụ: `ADD`, `SUB`, `AND`, `SRA`.

```text
rs1, rs2
  -> ALU
  -> wb_data = alu_data
  -> ghi rd
  -> PC + 4
```

Control chính:

```text
opa_sel = 0       // op_a = rs1
opb_sel = 0       // op_b = rs2
wb_sel  = 0       // write-back từ ALU
rd_wren = 1
mem_wren = 0
pc_sel = 0
```

### Nhóm B — ALU immediate: I-type

Ví dụ: `ADDI`, `SLTI`, `XORI`, `SRAI`.

```text
rs1 + I-immediate
  -> ALU
  -> wb_data = alu_data
  -> ghi rd
  -> PC + 4
```

Control chính:

```text
opa_sel = 0       // op_a = rs1
opb_sel = 1       // op_b = immediate
imm_sel = I
wb_sel  = 0
rd_wren = 1
pc_sel = 0
```

### Nhóm C — Load

Ví dụ: `LB`, `LH`, `LW`, `LBU`, `LHU`.

```text
rs1 + I-immediate
  -> ALU tạo địa chỉ
  -> DMEM đọc word
  -> LSU chọn byte/halfword/word và mở rộng
  -> wb_data = ld_data
  -> ghi rd
```

Control chính:

```text
opa_sel = 0
opb_sel = 1
imm_sel = I
alu_op = ADD
wb_sel  = 1
rd_wren = 1
mem_wren = 0
pc_sel = 0
```

### Nhóm D — Store

Ví dụ: `SB`, `SH`, `SW`.

```text
rs1 + S-immediate
  -> ALU tạo địa chỉ
rs2
  -> LSU tạo store data và byte mask
  -> DMEM ghi bộ nhớ
  -> PC + 4
```

Control chính:

```text
opa_sel = 0
opb_sel = 1
imm_sel = S
alu_op = ADD
mem_wren = 1
rd_wren = 0
pc_sel = 0
```

Store không có write-back về register file.

### Nhóm E — Conditional branch

Ví dụ: `BEQ`, `BNE`, `BLT`, `BGE`, `BLTU`, `BGEU`.

```text
rs1, rs2
  -> BRC kiểm tra điều kiện
PC + B-immediate
  -> ALU tạo branch target
```

Control chính:

```text
opa_sel = 1       // op_a = PC
opb_sel = 1       // op_b = B-immediate
imm_sel = B
alu_op = ADD
rd_wren = 0
mem_wren = 0
```

`pc_sel` bằng kết quả điều kiện branch và `instr_valid`:

```text
điều kiện đúng  -> PC_next = PC + offset
điều kiện sai   -> PC_next = PC + 4
```

Điểm quan trọng: BRC dùng `rs1/rs2` để so sánh; ALU dùng `PC/immediate`
để tính target. `opb_sel = 1` không có nghĩa BRC so sánh `rs1` với
immediate.

### Nhóm F — Unconditional jump

#### `JAL`

```text
PC + J-immediate -> ALU -> jump target
PC + 4 -> wb_data -> rd
```

Control chính:

```text
opa_sel = 1
opb_sel = 1
imm_sel = J
alu_op = ADD
pc_sel = 1
rd_wren = 1
wb_sel = 2
```

#### `JALR`

```text
rs1 + I-immediate -> ALU -> jump target
PC + 4 -> wb_data -> rd
```

Control chính:

```text
opa_sel = 0
opb_sel = 1
imm_sel = I
alu_op = ADD
pc_sel = 1
rd_wren = 1
wb_sel = 2
```

`JALR` chỉ hợp lệ khi `funct3 = 000`. Target cuối cùng được clear bit 0.

### Nhóm G — Upper immediate

#### `LUI`

```text
U-immediate -> wb_data -> rd
PC + 4
```

Control chính:

```text
imm_sel = U
wb_sel = 3
rd_wren = 1
pc_sel = 0
```

#### `AUIPC`

```text
PC + U-immediate -> ALU -> wb_data -> rd
PC + 4
```

Control chính:

```text
opa_sel = 1
opb_sel = 1
imm_sel = U
alu_op = ADD
wb_sel = 0
rd_wren = 1
pc_sel = 0
```

## 5. Immediate format theo nhóm

| Format | Instruction sử dụng | Cách tạo |
|---|---|---|
| I | ALU immediate, load, `JALR` | `sign_extend(instr[31:20])` |
| S | store | `sign_extend(instr[31:25] || instr[11:7])` |
| B | branch | `sign_extend({instr[31], instr[7], instr[30:25], instr[11:8], 1'b0})` |
| U | `LUI`, `AUIPC` | `{instr[31:12], 12'b0}` |
| J | `JAL` | `sign_extend({instr[31], instr[19:12], instr[20], instr[30:21], 1'b0})` |

Với B/J format, bit 0 của immediate luôn bằng `0`.

## 6. Write-back theo instruction

| `wb_sel` | Dữ liệu ghi | Instruction |
|---:|---|---|
| `0` | `alu_data` | R-type, I-type ALU, `AUIPC` |
| `1` | `ld_data` | Load |
| `2` | `PC + 4` | `JAL`, `JALR` |
| `3` | `imm` | `LUI` |

Branch và store không ghi register dù MUX vẫn tạo `wb_data`, vì
`rd_wren = 0`.

## 7. Bảng tổng hợp control nhanh

| Nhóm | `imm_sel` | `opa_sel` | `opb_sel` | `wb_sel` | `rd_wren` | `mem_wren` | `pc_sel` |
|---|---:|---:|---:|---:|---:|---:|---|
| R-type | - | 0 | 0 | 0 | 1 | 0 | 0 |
| ALU immediate | I | 0 | 1 | 0 | 1 | 0 | 0 |
| Load | I | 0 | 1 | 1 | 1 | 0 | 0 |
| Store | S | 0 | 1 | - | 0 | 1 | 0 |
| Branch | B | 1 | 1 | - | 0 | 0 | condition |
| `JAL` | J | 1 | 1 | 2 | 1 | 0 | 1 |
| `JALR` | I | 0 | 1 | 2 | 1 | 0 | 1 |
| `LUI` | U | - | - | 3 | 1 | 0 | 0 |
| `AUIPC` | U | 1 | 1 | 0 | 1 | 0 | 0 |

`-` nghĩa là tín hiệu không ảnh hưởng đến kết quả observable của instruction.

## 8. Checklist trả lời QA

Khi gặp một instruction, trả lời theo thứ tự:

1. Opcode của nó thuộc nhóm nào?
2. Format là R/I/S/B/U/J?
3. `funct3` chọn instruction con nào?
4. Có cần kiểm tra `funct7` không?
5. ALU nhận `rs1`, `rs2`, `PC` hay immediate?
6. ALU đang tính dữ liệu, địa chỉ hay branch target?
7. BRC có tham gia không?
8. Dữ liệu write-back lấy từ ALU, memory, `PC+4` hay immediate?
9. Có ghi `rd` không?
10. PC đi tới `PC+4` hay target?
