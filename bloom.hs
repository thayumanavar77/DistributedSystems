import Data.Bits
import Data.Word
import Data.List

type Bitarray32 = Word32
barr :: Word32
barr = 0
m = 32
k = 3

hash_fn x i = ((x^2+x^3)*i) `mod` m

bloom x = let hfn = hash_fn x in  map (hfn) [1,2,3]

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
  let xs' = [2010]
  let xs = [2010,2013,2007,2004]
  putStrLn $ show $ map (bloom) $ xs
  putStrLn $ show $ barr
  let barr' = insertBloom barr k 2010
  let barr'' =  foldl' (\bl x -> insertBloom bl k x)  barr xs
  putStrLn $ show $ barr'
  putStrLn $ show $ membQueryBloom barr' k 2010
  putStrLn $ show $ toBsl barr''
  putStrLn $ show $ bloom 3200 
