SELECT product_name, list_price
FROM northwinds.products
WHERE list_price IN (SELECT MIN(list_price) FROM product)
OR list_pricen IN (SELECT MAX(list_price) FROM products);


SELECT category, COUNT(*) as NumInStock
FROM northwinds.products
GROUP BY category
HAVING NumInStock < 5;


SELECT s.*, p.*
FROM suppliers s
JOIN products
ON s.id = p.supplier_ids;

SELECT CASE discontinued
			WHEN 1 THEN 'yes'
            ELSE 'no'
		END AS is_discontinued
		, COUNT(*) AS product_count
FROM products;