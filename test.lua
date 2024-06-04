-- Returns a table of function descriptions that can be used by Chewie to
--
return {

    -- First function
    {name = "getenv",
        -- Function description
        description = "Return the value of a specified environment variable",

        -- Function parameters
        parameters = {
            -- First parameter name
            {name = "varname",
                -- Description of the parameter
                description = "The environment variable to read",
                -- Type of the parameter
                type = "string",
                -- Whether the parameter is required (optional, default is false)
                required = true
            },
            -- Second parameter name
            {name = "default",
                description = "The default value to return if the environment variable is not set",
                type = "string",
                -- Possible values for the parameter (optional)
                choices = {"default", "empty", "nil"}
            }
        },

        -- Function implementation
        code = function(varname, default)
            res = os.getenv(varname);
            if (res == fail) then
                return default
            end
            return res
        end
    },

    -- Second function
    {name = "gettime",
        description = "Return the current time",
        code = function()
            return os.date()
        end
    }
}
