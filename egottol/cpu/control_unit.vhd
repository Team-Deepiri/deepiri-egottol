-- control_unit.vhd
-- Combinational control unit.  Decodes the 32-bit instruction word and
-- produces all datapath control signals plus the sign-extended immediate.
--
-- Instruction format (32-bit):
--   [31:27] opcode (5 bits)
--   [26:22] rd     (5 bits)
--   [21:17] rs1    (5 bits)
--   [16:12] rs2    (5 bits)
--   [11:0]  imm12  (12 bits)

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity control_unit is
    port (
        instr      : in  std_logic_vector(31 downto 0);
        -- register addresses
        rd         : out std_logic_vector(4 downto 0);
        rs1        : out std_logic_vector(4 downto 0);
        rs2        : out std_logic_vector(4 downto 0);
        -- sign-extended immediate
        imm        : out std_logic_vector(15 downto 0);
        -- control signals
        reg_write  : out std_logic;
        mem_read   : out std_logic;
        mem_write  : out std_logic;
        branch     : out std_logic;
        mem_to_reg : out std_logic;
        alu_src    : out std_logic;
        jump       : out std_logic;
        -- ALU operation (4-bit, matches opcode[3:0])
        alu_op     : out std_logic_vector(3 downto 0)
    );
end entity control_unit;

architecture rtl of control_unit is

    -- Opcode constants (5-bit)
    constant OPC_NOP : std_logic_vector(4 downto 0) := "00000";
    constant OPC_ADD : std_logic_vector(4 downto 0) := "00001";
    constant OPC_SUB : std_logic_vector(4 downto 0) := "00010";
    constant OPC_AND : std_logic_vector(4 downto 0) := "00011";
    constant OPC_OR  : std_logic_vector(4 downto 0) := "00100";
    constant OPC_XOR : std_logic_vector(4 downto 0) := "00101";
    constant OPC_NOT : std_logic_vector(4 downto 0) := "00110";
    constant OPC_SLT : std_logic_vector(4 downto 0) := "00111";
    constant OPC_SLL : std_logic_vector(4 downto 0) := "01000";
    constant OPC_SRL : std_logic_vector(4 downto 0) := "01001";
    constant OPC_SRA : std_logic_vector(4 downto 0) := "01010";
    constant OPC_MOV : std_logic_vector(4 downto 0) := "01011";
    constant OPC_LUI : std_logic_vector(4 downto 0) := "01100";
    constant OPC_JMP : std_logic_vector(4 downto 0) := "01101";
    constant OPC_BEQ : std_logic_vector(4 downto 0) := "01110";
    constant OPC_BNE : std_logic_vector(4 downto 0) := "01111";
    constant OPC_LW  : std_logic_vector(4 downto 0) := "10000";
    constant OPC_SW  : std_logic_vector(4 downto 0) := "10001";
    constant OPC_LI  : std_logic_vector(4 downto 0) := "10010";

    signal opcode  : std_logic_vector(4 downto 0);
    signal imm12   : std_logic_vector(11 downto 0);

begin

    -- Field extraction
    opcode <= instr(31 downto 27);
    rd     <= instr(26 downto 22);
    rs1    <= instr(21 downto 17);
    rs2    <= instr(16 downto 12);
    imm12  <= instr(11 downto 0);

    -- Sign-extend imm12 to 16 bits
    imm <= (15 downto 12 => imm12(11)) & imm12;

    -- Combinational control decode
    process(opcode)
    begin
        -- Default: all signals deasserted
        reg_write  <= '0';
        mem_read   <= '0';
        mem_write  <= '0';
        branch     <= '0';
        mem_to_reg <= '0';
        alu_src    <= '0';
        jump       <= '0';
        alu_op     <= "0000";

        case opcode is
            -- R-type arithmetic / logic (alu_src=0, uses rs2)
            when OPC_ADD =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0001";
            when OPC_SUB =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0010";
            when OPC_AND =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0011";
            when OPC_OR =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0100";
            when OPC_XOR =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0101";
            when OPC_SLT =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "0111";
            when OPC_SLL =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "1000";
            when OPC_SRL =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "1001";
            when OPC_SRA =>
                reg_write <= '1'; alu_src <= '0'; alu_op <= "1010";

            -- NOT: unary, uses rs1 only (alu_src=1 so datapath sends imm as B,
            --       but the ALU NOT op ignores B and inverts A)
            when OPC_NOT =>
                reg_write <= '1'; alu_src <= '1'; alu_op <= "0110";

            -- MOV / LI / LUI: write immediate to rd via ALU pass-through (MOV)
            when OPC_MOV =>
                reg_write <= '1'; alu_src <= '1'; alu_op <= "1011";
            when OPC_LI =>
                reg_write <= '1'; alu_src <= '1'; alu_op <= "1011";
            when OPC_LUI =>
                reg_write <= '1'; alu_src <= '1'; alu_op <= "1011";

            -- Load word: rd = MEM[rs1 + imm]
            when OPC_LW =>
                reg_write  <= '1';
                mem_read   <= '1';
                mem_to_reg <= '1';
                alu_src    <= '1';
                alu_op     <= "0001";   -- ADD to form effective address

            -- Store word: MEM[rs1 + imm] = rs2
            when OPC_SW =>
                mem_write <= '1';
                alu_src   <= '1';
                alu_op    <= "0001";   -- ADD to form effective address

            -- Branch equal / not-equal: subtract to set flags
            when OPC_BEQ =>
                branch  <= '1';
                alu_src <= '0';
                alu_op  <= "0010";

            when OPC_BNE =>
                branch  <= '1';
                alu_src <= '0';
                alu_op  <= "0010";

            -- Unconditional jump (absolute, uses imm12)
            when OPC_JMP =>
                jump <= '1';

            -- NOP and unrecognised: all signals remain at default (0)
            when others =>
                null;
        end case;
    end process;

end architecture rtl;
