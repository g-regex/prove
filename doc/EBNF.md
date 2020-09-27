### [prove] syntax

| $\langle$`expressions`$\rangle$ | $\Big\{$ $\langle$`chain`$\rangle$ $\Big\}$                  |
| :------------------------------ | :----------------------------------------------------------- |
| $\langle$`chain`$\rangle$       | $\bigg\{\Big(\langle$`statement`$\rangle\Big|\langle$`symbol`$\rangle\Big)\bigg\}$ $\langle$`statement`$\rangle$$\bigg\{\Big(\langle$`statement`$\rangle\Big|\langle$`symbol`$\rangle\Big)\bigg\}$ |
| $\langle$`statement`$\rangle$   | "["$\Big(\langle$`operand`$\rangle\Big|\langle$`chain`$\rangle\Big|$"false"$\Big)$"]" |
| $\langle$`symbol`$\rangle$      | ":"$\Big|$"\\"$\big($"a"$|\dots|$"z"$|$"A"$|\dots|$"Z"$\big)\Big\{\big($"a"$|\dots|$"z"$|$"A"$|\dots|$"Z"$|$"0"$|\dots|$"9"$|$"_"$\big)\Big\}$ |
| $\langle$`operand`$\rangle$     | $\langle$`number`$\rangle \Big| \big($"a"$|\dots|$"z"$|$"A"$|\dots|$"Z"$\big)\Big\{\big($"a"$|\dots|$"z"$|$"A"$|\dots|$"Z"$|$"0"$|\dots|$"9"$|$"_"$\big)\Big\}$ |
| $\langle$`number`$\rangle$      | $\big($"0"$|\dots|$"9"$\big)\Big\{\big($"0"$|\dots|$"9"$\big)\Big\}$ |

 $|$ 			indicates OR			      	  $\big(\dots\big)$ 			indicate requirement

$\Big[\dots\Big]$ 	indicate optionality			$\Big\{\dots\Big\}$			indicate zero or more repetitions

Spaces between tokens are ignored.

```mermaid
sequenceDiagram
    participant expressions
    participant chain
    participant statement
    participant symbol
    participant operand
    
   
    loop as long as at least one <statement> follows
    	expressions ->> chain: descend
    	loop as long as <statement>s or <symbol>s on same depth follow
    		rect rgb(240, 240, 240)
    		alt if <chain> starts with <statement>
    			rect rgb(230, 230, 230)
    			chain ->> statement: descend
    			rect rgb(255, 255, 255)
    			alt if <statement> is an <operand>
    				statement ->> operand: descend
    				operand -->> statement: return
    				statement -->> chain: return
    			else else if <statement> is "false"
    				statement -->> chain: return
    			else else if <statement> is another <chain>
    				statement ->> chain: descend
    				Note left of chain: Recursively process <chain>
    			end
    			end
    			end
    		else else if <chain> starts with <symbol>
    			chain ->> symbol: descend
    			symbol -->> chain: return
    		end
    		end
    	end
    	chain -->> expressions: return
    end
```
