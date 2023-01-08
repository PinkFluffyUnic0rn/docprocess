package main

import "fmt"
import "os"
import "bufio"
import "encoding/json"
import "strings"
import "strconv"

func main() {
	reader := bufio.NewReader(os.Stdin)

	fmt.Println(`<!DOCTYPE html>				
<html>								
<head>								
	<meta charset="utf-8">				
	<style>							
		table, th, td {					
			background-color: #DCDCDC;		
			border-collapse: collapse;		
			border: 1px solid black;		
			text-align: center;			
		}						
		.block {					
			word-wrap: break-word;			
			overflow: hidden;			
			text-overflow: ellipsis;		
			max-height: 200px;			
		}						
	</style>						
</head>		
<body>`)

	s := "";
	for {
		text, err := reader.ReadString('\n')

		if err != nil {
			break;
		}

		s = s + text
	}

	var m map[string]interface{}

	type valstring struct {
		valtype	string
		value	string
	}

	json.Unmarshal([]byte(s), &m)

	var rows float64
	var cols float64

	if vs, ok := m["rows"].(map[string]interface{}); ok {
		if rows, ok = vs["value"].(float64); ok {
		}
	}

	if vs, ok := m["cols"].(map[string]interface{}); ok {
		if cols, ok = vs["value"].(float64); ok {
		}
	}


	fmt.Printf("<table>\n");

	for i := 0; i < int(rows); i++ {
		row := ""
		for j := 0; j < int(cols); j++ {
			var t string
			var val string
			var hs float64
			var vs float64

			if r, ok := m["value"].([]interface{}); ok {
			if c, ok := r[i].([]interface{}); ok {
			if v, ok := c[j].(map[string]interface{}); ok {

			if t, ok = v["type"].(string); ok {
			}

			if t == "string" {
				if val, ok = v["value"].(string); ok {
				}
			} else if t == "int" {
				if vf, ok := v["value"].(float64); ok {
					val = strconv.Itoa(int(vf))
				}
			} else if t == "float" {
				if vf, ok := v["value"].(float64); ok {
					val = fmt.Sprintf("%f", vf);
				}
			}

			if vspan, ok := v["vspan"].(map[string]interface{}); ok {
			var ok bool
			if vs, ok = vspan["value"].(float64); ok {
			}}

			if hspan, ok := v["hspan"].(map[string]interface{}); ok {
			var ok bool
			if hs, ok = hspan["value"].(float64); ok {
			}}

			if hs < 0 || vs < 0 {
				continue
			}

			strings.Replace(val, "&", "\\&amp;", -1)
			strings.Replace(val, "\"", "\\&quot;", -1)
			strings.Replace(val, "'", "\\&apos;", -1)
			strings.Replace(val, "=", "\\&equal;", -1)
			strings.Replace(val, "<", "\\&lt;", -1)
			strings.Replace(val, ">", "\\&gt;", -1)
			strings.Replace(val, "\n", "<br>", -1)

			row = row + "<td"

			if vs != 0 {
				row = row + fmt.Sprintf(
					" rowspan=\"%d\"", int(vs))
			}

			if hs != 0 {
				row = row + fmt.Sprintf(
					" colspan=\"%d\"", int(hs))
			}

			row = row + ">" + val + fmt.Sprintf("</td>\n")

			}}}
		}

		if row != "" {
			fmt.Printf("<tr>\n%s</tr>\n", row)
		}
	}

	fmt.Printf("</table>\n");
	fmt.Printf("</body>\n");
}
