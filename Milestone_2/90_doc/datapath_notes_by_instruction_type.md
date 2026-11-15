# Datapath Notes by Instruction Type

Tài liệu lưu ý nhanh về datapath của từng nhóm instruction trong
`Milestone_2`.

## 1. Quy tắc chung

Mỗi instruction đi qua các bước chính:

```text
PC -> IMEM -> decode -> Regfile/ImmGen -> ALU/BRC/LSU
   -> write-back hoặc PC update
```

Khi phân tích một instruction, cần trả lời bốn câu hỏi:

1. ALU đang tính **kết quả**, **địa chỉ**, hay **PC target**?
2. Hai toán hạng ALU lấy từ `rs1`, `rs2`, `PC` hay immediate?
3. Có ghi `rd` hoặc ghi memory không?
4. PC đi tới `PC+4` hay địa chỉ target?

## 2. R-type: ALU giữa hai thanh ghi

Instruction tiêu biểu:

```text
ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND
```

### Luồng datapath

```text
rs1 ─────┐
         ├─> ALU ─> alu_data ─> wb_data ─> rd
rs2 ─────┘
```

### Control cần nhớ

```text
opa_sel = 0       // op_a = rs1
opb_sel = 0       // op_b = rs2
wb_sel  = 0       // write-back từ ALU
rd_wren = 1
mem_wren = 0
pc_sel = 0
```

### Lưu ý QA

- R-type dùng cả `rs1` và `rs2`; immediate không tham gia.
- `rd` nhận kết quả ALU.
- PC luôn tăng `4`.
- `SUB` không được nhận diện chỉ bằng `funct3`; cần `instruction[30]`.
- `ADD/SUB` có cùng `funct3 = 000`.
- `SRL/SRA` có cùng `funct3 = 101`.
- `funct7 = 0100000` chỉ hợp lệ cho `SUB` và `SRA`; không hợp lệ cho
  `AND`, `OR`, `XOR`, `SLT`, `SLTU`, `SLL`.

## 3. I-type ALU: ALU giữa thanh ghi và immediate

Instruction tiêu biểu:

```text
ADDI, SLTI, SLTIU, XORI, ORI, ANDI, SLLI, SRLI, SRAI
```

### Luồng datapath

```text
rs1 ───────────────┐
                   ├─> ALU ─> alu_data ─> wb_data ─> rd
I-immediate ───────┘
```

### Control cần nhớ

```text
opa_sel = 0       // op_a = rs1
opb_sel = 1       // op_b = I-immediate
imm_sel = I
wb_sel  = 0
rd_wren = 1
mem_wren = 0
pc_sel = 0
```

### Lưu ý QA

- `rs2` không được dùng; các bit `[24:20]` thuộc immediate hoặc shift
  amount tùy instruction.
- Immediate thường được sign-extend từ `instruction[31:20]`.
- Với `SLLI`, `SRLI`, `SRAI`, phần shift amount lấy từ các bit thấp của
  immediate và ALU chỉ dùng `op_b[4:0]`.
- `SRLI` và `SRAI` cùng `funct3 = 101`; bit `instruction[30]` phân biệt
  logical shift và arithmetic shift.
- `SLLI` yêu cầu `funct7 = 0000000`.
- Kết quả ALU được ghi vào `rd`, không truy cập DMEM.

## 4. Load: tính địa chỉ rồi đọc memory

Instruction:

```text
LB, LH, LW, LBU, LHU
```

### Luồng datapath

```text
rs1 + I-immediate
       |
       v
      ALU -> địa chỉ -> LSU -> DMEM -> dữ liệu 32 bit
                                  |
                                  v
                         chọn/extend -> ld_data -> rd
```

### Control cần nhớ

```text
opa_sel = 0
opb_sel = 1
imm_sel = I
alu_op = ADD
wb_sel  = 1       // wb_data = ld_data
rd_wren = 1
mem_wren = 0
pc_sel = 0
```

### Lưu ý QA

- ALU không tính giá trị cuối cùng của load; ALU chỉ tính **effective
  address**.
- `rs1` là base address, I-immediate là offset.
- `funct3` được truyền tới LSU để chọn kiểu dữ liệu.
- `LB/LH` sign-extend; `LBU/LHU` zero-extend.
- DMEM trả về word chứa địa chỉ; LSU chọn byte hoặc halfword bên trong
  word dựa trên các bit thấp của địa chỉ.
- Dữ liệu load đi qua `wb_sel = 1`, sau đó mới ghi vào `rd`.
- Load không ghi memory.

## 5. Store: tính địa chỉ và ghi memory

Instruction:

```text
SB, SH, SW
```

### Luồng datapath

```text
rs1 + S-immediate -> ALU -> địa chỉ memory
rs2 ---------------------> LSU -> store data + byte mask -> DMEM
```

### Control cần nhớ

```text
opa_sel = 0
opb_sel = 1
imm_sel = S
alu_op = ADD
rd_wren = 0
mem_wren = 1
pc_sel = 0
```

### Lưu ý QA

- `rs1` là base address.
- `rs2` là dữ liệu cần ghi, không phải toán hạng thứ hai của ALU trong
  datapath này.
- ALU dùng `rs1 + S-immediate` để tạo địa chỉ.
- Immediate S-type ghép từ `instruction[31:25]` và
  `instruction[11:7]`.
- `SB` ghi 1 byte, `SH` ghi 2 byte, `SW` ghi 4 byte.
- LSU tạo `byte mask`; DMEM chỉ cập nhật những byte có mask bằng `1`.
- Store không có write-back và không dùng `rd`.

## 6. Conditional branch: so sánh và tính target song song

Instruction:

```text
BEQ, BNE, BLT, BGE, BLTU, BGEU
```

### Luồng datapath

```text
                 ┌──> BRC: kiểm tra điều kiện
rs1, rs2 ────────┤
                 │
PC, B-immediate ─┴──> ALU: tính PC + offset
```

### Control cần nhớ

```text
opa_sel = 1       // op_a = PC
opb_sel = 1       // op_b = B-immediate
imm_sel = B
alu_op = ADD
rd_wren = 0
mem_wren = 0
```

### Lưu ý QA

- BRC và ALU có hai nhiệm vụ khác nhau:
  - BRC so sánh `rs1` với `rs2`.
  - ALU tính branch target `PC + offset`.
- `opb_sel = 1` chỉ áp dụng cho ALU; nó không làm BRC so sánh `rs1` với
  immediate.
- `BEQ/BNE` dùng `br_equal`.
- `BLT/BGE` dùng signed less-than.
- `BLTU/BGEU` dùng unsigned less-than.
- `br_unsigned = funct3[1]`.
- Branch không ghi `rd` và không ghi memory.
- Nếu điều kiện đúng:

  ```text
  pc_sel = 1
  PC_next = PC + B-immediate
  ```

- Nếu điều kiện sai:

  ```text
  pc_sel = 0
  PC_next = PC + 4
  ```

- B-immediate có bit 0 bằng `0`; offset được sign-extend.

## 7. JAL: jump tương đối theo PC

Instruction:

```text
JAL rd, offset
```

### Luồng datapath

```text
PC + J-immediate -> ALU -> jump target -> PC
PC + 4 -----------------------------> rd
```

### Control cần nhớ

```text
opa_sel = 1
opb_sel = 1
imm_sel = J
alu_op = ADD
pc_sel = 1
wb_sel = 2       // wb_data = PC + 4
rd_wren = 1
mem_wren = 0
```

### Lưu ý QA

- `JAL` vừa thay đổi PC vừa ghi register.
- `rd` nhận địa chỉ quay về `PC+4`, không nhận jump target.
- Jump target là `PC + J-immediate`.
- J-immediate được sign-extend và bit 0 bằng `0`.
- Nếu `rd = x0`, target vẫn được nhảy nhưng return address bị bỏ qua vì
  register file không ghi `x0`.

## 8. JALR: jump gián tiếp qua thanh ghi

Instruction:

```text
JALR rd, offset(rs1)
```

### Luồng datapath

```text
rs1 + I-immediate -> ALU -> jump target -> PC
PC + 4 -----------------------------> rd
```

### Control cần nhớ

```text
opa_sel = 0
opb_sel = 1
imm_sel = I
alu_op = ADD
pc_sel = 1
wb_sel = 2
rd_wren = 1
```

### Lưu ý QA

- Khác `JAL`, `JALR` lấy base từ `rs1`, không phải PC.
- `JALR` sử dụng I-immediate.
- Trong controller này, `JALR` chỉ hợp lệ khi `funct3 = 000`.
- Target cuối cùng được clear bit 0:

  ```systemverilog
  pc_next = {alu_data[31:1], 1'b0};
  ```

- `rd` nhận `PC+4`.

## 9. LUI: nạp upper immediate

Instruction:

```text
LUI rd, imm20
```

### Luồng datapath

```text
U-immediate -> write-back MUX -> rd
```

### Control cần nhớ

```text
imm_sel = U
wb_sel = 3       // wb_data = imm
rd_wren = 1
pc_sel = 0
```

### Lưu ý QA

- `LUI` không cần `rs1`, `rs2` hoặc ALU.
- U-immediate được tạo bằng:

  ```text
  {instruction[31:12], 12'b0}
  ```

- `rd` nhận trực tiếp immediate đã dịch trái 12 bit.
- PC vẫn đi tới `PC+4`.

## 10. AUIPC: cộng PC với upper immediate

Instruction:

```text
AUIPC rd, imm20
```

### Luồng datapath

```text
PC ─────────────┐
                ├─> ALU ADD -> wb_data -> rd
U-immediate ────┘
```

### Control cần nhớ

```text
opa_sel = 1       // op_a = PC
opb_sel = 1       // op_b = U-immediate
imm_sel = U
alu_op = ADD
wb_sel = 0
rd_wren = 1
pc_sel = 0
```

### Lưu ý QA

- `AUIPC` dùng PC hiện tại, không dùng `PC+4`.
- Kết quả ghi vào `rd` là `PC + U-immediate`.
- Đây là instruction tính toán, không phải jump; PC vẫn là `PC+4`.

## 11. Bảng so sánh nhanh

| Nhóm | ALU input A | ALU input B | ALU purpose | Write-back | PC |
|---|---|---|---|---|---|
| R-type | `rs1` | `rs2` | Kết quả ALU | `alu_data -> rd` | `PC+4` |
| I-type ALU | `rs1` | I-imm | Kết quả ALU | `alu_data -> rd` | `PC+4` |
| Load | `rs1` | I-imm | Địa chỉ | `ld_data -> rd` | `PC+4` |
| Store | `rs1` | S-imm | Địa chỉ | Không có | `PC+4` |
| Branch | `PC` | B-imm | Branch target | Không có | Target hoặc `PC+4` |
| `JAL` | `PC` | J-imm | Jump target | `PC+4 -> rd` | Target |
| `JALR` | `rs1` | I-imm | Jump target | `PC+4 -> rd` | Target |
| `LUI` | Không cần | Không cần | Không dùng | U-imm -> `rd` | `PC+4` |
| `AUIPC` | `PC` | U-imm | Kết quả PC-relative | `alu_data -> rd` | `PC+4` |

## 12. Các lỗi trình bày thường gặp

- Nhầm BRC với ALU: BRC so sánh branch operands; ALU tính target.
- Nói `BEQ` dùng ALU để so sánh `rs1` và `rs2`; trong datapath này việc đó
  do BRC thực hiện.
- Nói load lấy trực tiếp `rs1` làm địa chỉ; thực tế địa chỉ là
  `rs1 + I-immediate`.
- Nói store ghi `rd`; store không có write-back và `rd_wren = 0`.
- Nhầm `JAL` ghi jump target vào `rd`; giá trị ghi là `PC+4`.
- Nhầm `JALR` dùng J-immediate; `JALR` dùng I-immediate.
- Nhầm `AUIPC` là jump; AUIPC chỉ ghi `PC + U-immediate`, còn PC vẫn
  tăng `4`.
- Nhầm `o_opb_sel = 1` của branch là BRC dùng immediate; signal này chỉ
  chọn toán hạng B của ALU.
- Quên rằng `LUI` lấy immediate trực tiếp qua write-back MUX, không cần
  ALU.
- Quên kiểm tra `instruction[30]` cho `SUB`, `SRA`, `SRAI`.

