-- register_file.vhd
-- 32 x 16-bit register file.
-- x0 is hardwired to zero (writes to x0 are silently ignored).
-- Reads are asynchronous; writes are synchronous on the rising clock edge.
-- Reset is synchronous, active-high.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity register_file is
    port (
        clk : in  std_logic;
        rst : in  std_logic;
        we  : in  std_logic;
        rs1 : in  std_logic_vector(4 downto 0);
        rs2 : in  std_logic_vector(4 downto 0);
        rd  : in  std_logic_vector(4 downto 0);
        wd  : in  std_logic_vector(15 downto 0);
        rd1 : out std_logic_vector(15 downto 0);
        rd2 : out std_logic_vector(15 downto 0)
    );
end entity register_file;

architecture rtl of register_file is

    type reg_array_t is array(0 to 31) of std_logic_vector(15 downto 0);
    signal regs : reg_array_t := (others => (others => '0'));

begin

    -- Synchronous write with synchronous reset
    process(clk)
    begin
        if rising_edge(clk) then
            if rst = '1' then
                regs <= (others => (others => '0'));
            elsif we = '1' and rd /= "00000" then
                regs(to_integer(unsigned(rd))) <= wd;
            end if;
        end if;
    end process;

    -- Asynchronous read; x0 always returns 0
    rd1 <= (others => '0') when rs1 = "00000" else regs(to_integer(unsigned(rs1)));
    rd2 <= (others => '0') when rs2 = "00000" else regs(to_integer(unsigned(rs2)));

end architecture rtl;
