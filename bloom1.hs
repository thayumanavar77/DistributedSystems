import Data.Bits
import Data.Word
import Data.List

type Bitarray32 = Word32
barr :: Word32
barr = 0
m = 32
k = 3

hash_fn x i = ((x^2+x^3)*i) `mod` m

insertBloom barr' k x = let
                  hfn = hash_fn x
                  bitslots= map (hfn) [1..k]
                in
                  foldl' (\bx bs -> bx `setBit` (bs)) barr' bitslots

membQueryBloom barr' k x = let
                            hfn = hash_fn x
                            bitSlots= map (hfn) [1..k]
                           in
                            foldl' (\qt bs -> qt && (barr' `testBit` (bs))) True bitSlots

toBsl barr' = foldl' (\bsl bs -> if testBit barr' (bs) then bs:bsl else bsl) [] $ reverse [0..31]

main = do
  let xs = [2010,2013,2007,2004]
  putStrLn $ "Inserting elements " ++ (intercalate ", " $ map (show) xs) ++ " in Bloom filter."
  let barr'' =  foldl' (\bl x -> insertBloom bl k x)  barr xs
  putStrLn $ "Membership query for element 2010 in Bloom filter:" ++ (show $ membQueryBloom  barr''  k 2010)
  putStrLn $ "Membership query for element 2007 in Bloom filter:" ++ (show $ membQueryBloom  barr''  k 2007)
  putStrLn $ "Membership query for element 3200 in Bloom filter:(FALSE POSITIVE)" ++ (show $ membQueryBloom barr''  k 3200) 
  putStrLn $ "Membership query for element 2009 in Bloom filter:" ++ (show $ membQueryBloom barr''  k 2009) 
  putStrLn $ "Bit Slot positions in the bit arraty :" ++ (show $ toBsl barr'')
