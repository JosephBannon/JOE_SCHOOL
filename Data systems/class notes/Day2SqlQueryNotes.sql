SELECT p.product_name AS 'product name',
	s.company as 'company name'
FROM products p
INNER JOIN suppliers s 
ON s.id = p.id;

SELECT o.* FROM
(
	SELECT id as order_id, customer_id, order_date
	FROM orders
) AS o
ORDER BY order_date DESC;

-- using table aliasing , common table expression
WITH OrdersCTE AS 
(
	SELECT id as order_id, customer_id, order_date
	FROM orders
)
SELECT * FROM OrdersCTE
ORDER BY order_date DESC; # only runs in the batch

-- aliasing with column name definitions
WITH OrdersCTE(order_id, customer_id, order_date) AS 
(
	SELECT id, customer_id, order_date
	FROM orders
)
SELECT * FROM OrdersCTE
ORDER BY order_date DESC; 

-- take advantage of CTE like function calls
SELECT COUNT(*) as 'number of customers with orders' FROM
(
	SELECT c.id AS customer_id, c.last_name,
			 o.id AS order_id, o.order_date
	FROM customers c
    LEFT OUTER JOIN orders o #
    ON o.customer_id = c.id
) as co;
-- 
WITH CustomerOrdersCTE AS (
	SELECT c.id AS customer_id, c.last_name,
			 o.id AS order_id, o.order_date
	FROM customers c
    INNER JOIN orders o
    ON o.customer_id = c.id
)
SELECT COUNT(*) as 'number of customer with orders' FROM CustomerOrdersCTE;



-- union horizantal partion

SELECT COUNT(*) FROM (
	SELECT city, state_province
    FROM employees
    UNION -- allows for getting rid of duplicates in the return set
    SELECT city, state_province
    FROM  customers
)
AS distinct_addresses;

SELECT id as order_id, customer_id, order_date
FROM orders as o1
WHERE order_date =
	( SELECT MAX(order_date)
		FROM orders as o2
        WHERE o2.customer_id = o1.customer_id)
ORDER BY customer_id;

-- get the most recent order date from on table 
-- for o1 each , get order date and customer id
-- in o2 return max order date for customer id that is the same customer in row of o1
