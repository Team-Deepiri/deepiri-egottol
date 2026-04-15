-- memory.vhd
-- Two separate memory components:
--   imem : Instruction ROM  – 1024 x 32-bit, asynchronous read.
--   dmem : Data RAM         – 1024 x 16-bit, synchronous write / asynchronous read.
--
-- In a real flow the ROM contents would be initialised via a package or
-- attribute.  Here both memories power-up to all-zeros, which is correct
-- for simulation and for synthesis flows that support initialised BRAMs.

library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

-- ---------------------------------------------------------------------------
-- imem – Instruction ROM (1024 x 32-bit, asynchronous read)
-- ---------------------------------------------------------------------------
entity imem is
    port (
        addr  : in  std_logic_vector(9 downto 0);
        instr : out std_logic_vector(31 downto 0)
    );
end entity imem;

architecture rtl of imem is
    type rom_t is array(0 to 1023) of std_logic_vector(31 downto 0);
    signal rom : rom_t := (others => (others => '0'));
begin
    instr <= rom(to_integer(unsigned(addr)));
end architecture rtl;


-- ---------------------------------------------------------------------------
-- dmem – Data RAM (1024 x 16-bit, synchronous write / asynchronous read)
-- ---------------------------------------------------------------------------
library ieee;
use ieee.std_logic_1164.all;
use ieee.numeric_std.all;

entity dmem is
    port (
        clk  : in  std_logic;
        we   : in  std_logic;
        addr : in  std_logic_vector(9 downto 0);
        din  : in  std_logic_vector(15 downto 0);
        dout : out std_logic_vector(15 downto 0)
    );
end entity dmem;

architecture rtl of dmem is
    type ram_t is array(0 to 1023) of std_logic_vector(15 downto 0);
    signal ram : ram_t := (others => (others => '0'));
begin

    -- Synchronous write
    process(clk)
    begin
        if rising_edge(clk) then
            if we = '1' then
                ram(to_integer(unsigned(addr))) <= din;
            end if;
        end if;
    end process;

    -- Asynchronous read
    dout <= ram(to_integer(unsigned(addr)));

end architecture rtl;
